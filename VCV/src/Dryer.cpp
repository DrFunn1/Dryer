#include "plugin.hpp"
#include "DryerPhysics.hpp"

// Physics update rate — ~250Hz matches JS 4-substep × 60fps model
static const float PHYSICS_RATE = 250.f;

// MIDI note assignment — matches JS DryerAudio scale logic
// Base note C2 = MIDI 36; default scale [3,4] (minor 3rds+4ths)
static const int   SCALE_VEC[]  = {3, 4};
static const int   SCALE_LEN    = 2;
static const int   BASE_NOTE    = 36;   // C2
static const float VCV_REF_NOTE = 60.f; // C4 = 0V in VCV Rack

static int noteForSurface(int idx) {
    int note = BASE_NOTE;
    for (int i = 0; i < idx; i++)
        note += SCALE_VEC[i % SCALE_LEN];
    return note;
}

static float midiToVPerOct(int midiNote) {
    return (midiNote - VCV_REF_NOTE) / 12.f;
}

static int velocityFromImpact(float mps) {
    int v = (int)(mps * 300.f);
    return v > 127 ? 127 : (v < 1 ? 1 : v);
}


// ---------------------------------------------------------------------------
// Simple centered panel text widget (SVG text is ignored by nanosvg in VCV)
// ---------------------------------------------------------------------------
struct PanelText : Widget {
    std::string text;
    NVGcolor    color;
    float       fontSize;

    PanelText() : color(nvgRGB(0xaa, 0xaa, 0xaa)), fontSize(9.f) {}

    void draw(const DrawArgs& args) override {
        if (text.empty()) return;
        std::shared_ptr<Font> font =
            APP->window->loadFont(asset::system("res/fonts/ShareTechMono-Regular.ttf"));
        if (!font || font->handle < 0) return;
        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, fontSize);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(args.vg, color);
        nvgText(args.vg, 0.f, 0.f, text.c_str(), NULL);
    }
};

static PanelText* panelText(float cx_mm, float cy_mm, const char* text,
                             NVGcolor color = nvgRGB(0xaa, 0xaa, 0xaa),
                             float fontSize = 9.f) {
    PanelText* w = new PanelText;
    w->box.pos  = mm2px(Vec(cx_mm, cy_mm));
    w->box.size = Vec(0.f, 0.f);
    w->text     = text;
    w->color    = color;
    w->fontSize = fontSize;
    return w;
}


// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------
struct DryerModule : Module {

    enum ParamId {
        PARAM_SPEED,    // RPM
        PARAM_SIZE,     // drum diameter cm
        PARAM_NUMBER,   // vane count (integer)
        PARAM_HEIGHT,   // vane height % (integer)
        PARAM_MOON,
        PARAM_LINT,
        PARAMS_LEN
    };

    enum InputId {
        INPUT_SPEED_CV,
        INPUT_SIZE_CV,
        INPUT_NUMBER_CV,
        INPUT_HEIGHT_CV,
        INPUTS_LEN
    };

    enum OutputId {
        OUTPUT_CLK,     // square wave: RPM × 4 / 60 Hz, 50% duty
        OUTPUT_TRG,     // 10ms gate pulse on each collision
        OUTPUT_CV,      // V/oct pitch of last collision surface
        OUTPUT_POLY,    // Polyphonic: ch N = V/oct for surface N
        OUTPUTS_LEN
    };

    enum LightId {
        LIGHT_MOON,
        LIGHT_LINT,
        LIGHTS_LEN
    };

    DryerPhysics physics;

    // MIDI
    midi::Output     midiOutput;
    midi::InputQueue midiInput;

    // Per-surface state
    float noteOffTimer[DRYER_MAX_SURFACES] = {};
    int   activeNote[DRYER_MAX_SURFACES]   = {};
    float trgPulse[DRYER_MAX_SURFACES]     = {};
    static const float TRG_PULSE_TIME;
    static const float NOTE_DURATION;

    // CLK square wave phase accumulator (0–1)
    float clkPhase = 0.f;

    // Physics decimation
    float physicsAccum = 0.f;
    float physicsDt    = 1.f / PHYSICS_RATE;

    // Last pitch output
    float lastPitchV = 0.f;

    // CC override (-1 = inactive)
    float ccSpeed = -1.f, ccSize = -1.f, ccNumber = -1.f, ccHeight = -1.f;

    // Debug logging throttle (~1 log line per second)
    int debugFrameCount = 0;

    DryerModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        configParam(PARAM_SPEED,    0.f,  35.f, 18.f, "Speed",  " RPM");
        configParam(PARAM_SIZE,    40.f,  80.f, 60.f, "Size",   " cm");
        configParam(PARAM_NUMBER,   1.f,   7.f,  4.f, "Number of Vanes");
        configParam(PARAM_HEIGHT,  10.f,  50.f, 30.f, "Height", "%");
        configSwitch(PARAM_MOON, 0.f, 1.f, 0.f, "Moon Gravity", {"Earth", "Moon"});
        configSwitch(PARAM_LINT, 0.f, 1.f, 0.f, "Lint Trap",    {"Off",   "On"});

        // Snap all four knobs to integers (detented dryer-style; speed 0 = off)
        getParamQuantity(PARAM_SPEED)->snapEnabled  = true;
        getParamQuantity(PARAM_SIZE)->snapEnabled   = true;
        getParamQuantity(PARAM_NUMBER)->snapEnabled = true;
        getParamQuantity(PARAM_HEIGHT)->snapEnabled = true;

        configInput(INPUT_SPEED_CV,  "Speed CV (±5V)");
        configInput(INPUT_SIZE_CV,   "Size CV (±5V)");
        configInput(INPUT_NUMBER_CV, "Number CV (±5V)");
        configInput(INPUT_HEIGHT_CV, "Height CV (±5V)");

        configOutput(OUTPUT_CLK,  "Clock — square wave at Speed × 4/60 Hz");
        configOutput(OUTPUT_TRG,  "Trigger — 10ms gate per collision");
        configOutput(OUTPUT_CV,   "Pitch CV (V/oct, last surface)");
        configOutput(OUTPUT_POLY, "Polyphonic pitch (one channel per surface)");

        memset(activeNote, -1, sizeof(activeNote));
    }

    void process(const ProcessArgs& args) override {

        // --- MIDI CC input ---
        {
            midi::Message msg;
            while (midiInput.tryPop(&msg, args.frame)) {
                if ((msg.bytes[0] & 0xF0) == 0xB0) {
                    float norm = msg.bytes[2] / 127.f;
                    switch (msg.bytes[1]) {
                        case 1: ccSpeed  = norm; break;
                        case 2: ccSize   = norm; break;
                        case 3: ccNumber = norm; break;
                        case 4: ccHeight = norm; break;
                        default: break;
                    }
                }
            }
        }

        // --- Knobs + CV ---
        auto knobOrCC = [&](int id, float lo, float hi, float cc) -> float {
            return (cc >= 0.f) ? (lo + cc * (hi - lo)) : params[id].getValue();
        };

        float speed  = knobOrCC(PARAM_SPEED,  0.f,  35.f, ccSpeed);
        float size   = knobOrCC(PARAM_SIZE,   40.f, 80.f, ccSize);
        float number = knobOrCC(PARAM_NUMBER, 1.f,  7.f,  ccNumber);
        float height = knobOrCC(PARAM_HEIGHT, 10.f, 50.f, ccHeight);

        if (inputs[INPUT_SPEED_CV].isConnected())
            speed  = clamp(speed  + inputs[INPUT_SPEED_CV].getVoltage()  * (35.f/10.f), 0.f,  35.f);
        if (inputs[INPUT_SIZE_CV].isConnected())
            size   = clamp(size   + inputs[INPUT_SIZE_CV].getVoltage()   * (40.f/10.f), 40.f, 80.f);
        if (inputs[INPUT_NUMBER_CV].isConnected())
            number = clamp(number + inputs[INPUT_NUMBER_CV].getVoltage() * (6.f /10.f), 1.f,  7.f);
        if (inputs[INPUT_HEIGHT_CV].isConnected())
            height = clamp(height + inputs[INPUT_HEIGHT_CV].getVoltage() * (40.f/10.f), 10.f, 50.f);

        const bool moonOn = params[PARAM_MOON].getValue() > 0.5f;
        const bool lintOn = params[PARAM_LINT].getValue() > 0.5f;

        // --- Debug logging: ~1 line/sec ---
        if (++debugFrameCount >= (int)args.sampleRate) {
            debugFrameCount = 0;
            float cvS = inputs[INPUT_SPEED_CV].isConnected()  ? inputs[INPUT_SPEED_CV].getVoltage()  : 0.f;
            float cvSz= inputs[INPUT_SIZE_CV].isConnected()   ? inputs[INPUT_SIZE_CV].getVoltage()   : 0.f;
            float cvN = inputs[INPUT_NUMBER_CV].isConnected() ? inputs[INPUT_NUMBER_CV].getVoltage() : 0.f;
            float cvH = inputs[INPUT_HEIGHT_CV].isConnected() ? inputs[INPUT_HEIGHT_CV].getVoltage() : 0.f;
            DEBUG("DRYER speed=%4.1f(knob+cv=%.2fV)->%d  size=%4.1f(cv=%.2fV)->%d  number=%.2f(cv=%.2fV)->%d  height=%.2f(cv=%.2fV)->%d  surfs=%d",
                speed,  cvS,  (int)roundf(speed),
                size,   cvSz, (int)roundf(size),
                number, cvN,  (int)roundf(number),
                height, cvH,  (int)roundf(height),
                physics.surfaceCount);
        }

        physics.setParameters(speed, size, (int)roundf(number), (float)roundf(height));
        physics.moonGravity = moonOn;
        physics.lintTrap    = lintOn;

        lights[LIGHT_MOON].setBrightness(moonOn ? 1.f : 0.f);
        lights[LIGHT_LINT].setBrightness(lintOn ? 1.f : 0.f);

        // --- CLK: RPM-based square wave (speed × 4 / 60 Hz) ---
        // 20 RPM → 80/60 Hz → 80 BPM equivalent
        if (speed > 0.f) {
            float clkFreq = speed * 4.f / 60.f;
            clkPhase += clkFreq * args.sampleTime;
            if (clkPhase >= 1.f) clkPhase -= 1.f;
        }
        outputs[OUTPUT_CLK].setVoltage((speed > 0.f && clkPhase < 0.5f) ? 10.f : 0.f);

        // --- Physics step (decimated) ---
        physicsAccum += args.sampleTime;
        if (physicsAccum >= physicsDt) {
            physicsAccum -= physicsDt;
            physics.step(physicsDt);

            for (int c = 0; c < physics.pendingCount; c++) {
                const DryerCollision& col = physics.pendingCollisions[c];
                int surfIdx = -1;
                for (int s = 0; s < physics.surfaceCount; s++) {
                    if (&physics.surfaces[s] == col.surface) { surfIdx = s; break; }
                }
                if (surfIdx < 0) continue;

                const int   midiNote = noteForSurface(surfIdx);
                lastPitchV           = midiToVPerOct(midiNote);
                trgPulse[surfIdx]    = TRG_PULSE_TIME;

                const int vel = velocityFromImpact(col.velocity);
                if (activeNote[surfIdx] >= 0) {
                    midi::Message off;
                    off.setSize(3);
                    off.bytes[0] = 0x80;
                    off.bytes[1] = (uint8_t)activeNote[surfIdx];
                    off.bytes[2] = 0;
                    midiOutput.sendMessage(off);
                }
                midi::Message on;
                on.setSize(3);
                on.bytes[0] = 0x90;
                on.bytes[1] = (uint8_t)midiNote;
                on.bytes[2] = (uint8_t)vel;
                midiOutput.sendMessage(on);
                activeNote[surfIdx]   = midiNote;
                noteOffTimer[surfIdx] = NOTE_DURATION + (vel / 127.f) * 0.2f;
            }
        }

        // --- Tick timers, set outputs ---
        bool anyTrg = false;
        outputs[OUTPUT_POLY].setChannels(physics.surfaceCount);

        for (int s = 0; s < physics.surfaceCount; s++) {
            if (trgPulse[s] > 0.f) { trgPulse[s] -= args.sampleTime; anyTrg = true; }

            if (noteOffTimer[s] > 0.f) {
                noteOffTimer[s] -= args.sampleTime;
                if (noteOffTimer[s] <= 0.f && activeNote[s] >= 0) {
                    midi::Message off;
                    off.setSize(3);
                    off.bytes[0] = 0x80;
                    off.bytes[1] = (uint8_t)activeNote[s];
                    off.bytes[2] = 0;
                    midiOutput.sendMessage(off);
                    activeNote[s] = -1;
                }
            }

            outputs[OUTPUT_POLY].setVoltage(midiToVPerOct(noteForSurface(s)), s);
        }

        outputs[OUTPUT_TRG].setVoltage(anyTrg ? 10.f : 0.f);
        outputs[OUTPUT_CV].setVoltage(lastPitchV);
    }

    void onReset() override {
        for (int s = 0; s < physics.surfaceCount; s++) {
            if (activeNote[s] >= 0) {
                midi::Message off;
                off.setSize(3); off.bytes[0] = 0x80;
                off.bytes[1] = (uint8_t)activeNote[s]; off.bytes[2] = 0;
                midiOutput.sendMessage(off);
            }
        }
        physics.reset();
        memset(trgPulse,     0, sizeof(trgPulse));
        memset(noteOffTimer, 0, sizeof(noteOffTimer));
        memset(activeNote,  -1, sizeof(activeNote));
        lastPitchV = 0.f; physicsAccum = 0.f; clkPhase = 0.f;
        ccSpeed = ccSize = ccNumber = ccHeight = -1.f;
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();
        json_object_set_new(rootJ, "midiOutput", midiOutput.toJson());
        json_object_set_new(rootJ, "midiInput",  midiInput.toJson());
        return rootJ;
    }
    void dataFromJson(json_t* rootJ) override {
        json_t* j;
        if ((j = json_object_get(rootJ, "midiOutput"))) midiOutput.fromJson(j);
        if ((j = json_object_get(rootJ, "midiInput")))  midiInput.fromJson(j);
    }
};

const float DryerModule::TRG_PULSE_TIME = 0.010f;
const float DryerModule::NOTE_DURATION  = 0.150f;


// ---------------------------------------------------------------------------
// Widget
// ---------------------------------------------------------------------------
struct DryerWidget : ModuleWidget {
    DryerWidget(DryerModule* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Dryer.svg")));

        // Screws — standard pixel coords (RACK_GRID_WIDTH=15px, RACK_GRID_HEIGHT=380px)
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // --- Outputs: CLK=18, TRG=33, CV=48, POLY=63  Y=18.5mm ---
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(18.f, 18.5f)), module, DryerModule::OUTPUT_CLK));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(33.f, 18.5f)), module, DryerModule::OUTPUT_TRG));
        addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(48.f, 18.5f)), module, DryerModule::OUTPUT_CV));
        // OUTPUT_POLY: no VCV port widget — shown as USB-B artwork on panel SVG only

        // --- Jack labels (Y=12.5mm) ---
        NVGcolor labelColor = nvgRGB(0xaa, 0xaa, 0xaa);
        NVGcolor greenColor = nvgRGB(0x00, 0xff, 0x88);
        NVGcolor cyanColor  = nvgRGB(0x4e, 0xcd, 0xc4);
        NVGcolor goldColor  = nvgRGB(0xff, 0xe6, 0x6d);
        addChild(panelText(18.f, 12.5f, "CLK",  labelColor, 9.f));
        addChild(panelText(33.f, 12.5f, "TRG",  labelColor, 9.f));
        addChild(panelText(48.f, 12.5f, "CV",   labelColor, 9.f));
        addChild(panelText(63.f, 12.5f, "MIDI", goldColor,  9.f));

        // --- Latching buttons (Y=30.5mm) ---
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<GreenLight>>>(
            mm2px(Vec(9.f,    30.5f)), module, DryerModule::PARAM_MOON, DryerModule::LIGHT_MOON));
        addParam(createLightParamCentered<VCVLightLatch<MediumSimpleLight<GreenLight>>>(
            mm2px(Vec(72.5f,  30.5f)), module, DryerModule::PARAM_LINT, DryerModule::LIGHT_LINT));

        // Button labels (Y=38.5mm)
        addChild(panelText( 9.f,  38.5f, "MOON", nvgRGB(0xa8, 0xe6, 0xcf), 8.f));
        addChild(panelText(72.5f, 38.5f, "LINT", nvgRGB(0xff, 0xd3, 0xb6), 8.f));

        // --- Knobs: SPEED=10, SIZE=25, NUMBER=56, HEIGHT=71  Y=76.5mm ---
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(10.f,  76.5f)), module, DryerModule::PARAM_SPEED));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.f,  76.5f)), module, DryerModule::PARAM_SIZE));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(56.f,  76.5f)), module, DryerModule::PARAM_NUMBER));
        addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(71.f,  76.5f)), module, DryerModule::PARAM_HEIGHT));

        // Knob labels (Y=84mm)
        addChild(panelText(10.f, 84.f, "SPEED",  labelColor, 8.f));
        addChild(panelText(25.f, 84.f, "SIZE",   labelColor, 8.f));
        addChild(panelText(56.f, 84.f, "NUMBER", labelColor, 7.5f));
        addChild(panelText(71.f, 84.f, "HEIGHT", labelColor, 7.5f));

        // --- CV Inputs: same X, Y=94.5mm ---
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(10.f, 94.5f)), module, DryerModule::INPUT_SPEED_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(25.f, 94.5f)), module, DryerModule::INPUT_SIZE_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(56.f, 94.5f)), module, DryerModule::INPUT_NUMBER_CV));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(71.f, 94.5f)), module, DryerModule::INPUT_HEIGHT_CV));

        // CV labels (Y=100mm)
        addChild(panelText(10.f, 100.f, "CV", cyanColor, 7.f));
        addChild(panelText(25.f, 100.f, "CV", cyanColor, 7.f));
        addChild(panelText(56.f, 100.f, "CV", cyanColor, 7.f));
        addChild(panelText(71.f, 100.f, "CV", cyanColor, 7.f));

        // Group labels (Y=109mm)
        addChild(panelText(17.5f, 109.f, "DRUM",  greenColor, 10.f));
        addChild(panelText(63.5f, 109.f, "VANES", greenColor, 10.f));

        // Title — top of panel (SVG text ignored by VCV; C++ widget is what renders)
        addChild(panelText(40.64f,  7.0f, "DRYER",  greenColor,             9.f));
        addChild(panelText(40.64f, 124.f, "DRFUNN", nvgRGB(0x44,0x44,0x44), 7.f));
    }

    void appendContextMenu(Menu* menu) override {
        auto* module = dynamic_cast<DryerModule*>(this->module);
        if (!module) return;
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("MIDI Output (note + velocity on collision)"));
        appendMidiMenu(menu, &module->midiOutput);
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuLabel("MIDI Input (CC 1=Speed 2=Size 3=Number 4=Height)"));
        appendMidiMenu(menu, &module->midiInput);
    }
};

Model* modelDryer = createModel<DryerModule, DryerWidget>("Dryer");
