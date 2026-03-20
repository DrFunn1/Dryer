# Step-by-Step: Push to GitHub (Visual Guide)

## ⚠️ Important: Create Repo on GitHub FIRST!

```
┌─────────────────────────────────────────────────────────────┐
│  STEP 1: Create GitHub Repository                          │
└─────────────────────────────────────────────────────────────┘

1. Open browser → https://github.com/new

2. Fill in:
   Repository name:   dryer-pi
   Description:       Eurorack percussion generator
   Public/Private:    [Choose one]
   
3. ❌ UNCHECK all boxes:
   [ ] Add a README file
   [ ] Add .gitignore  
   [ ] Choose a license
   
4. Click [Create repository]

5. 📋 COPY the URL shown:
   https://github.com/YOUR_USERNAME/dryer-pi.git
   
   ⚠️ Keep this URL! You'll need it in Step 2.


┌─────────────────────────────────────────────────────────────┐
│  STEP 2: Initialize Local Git                              │
└─────────────────────────────────────────────────────────────┘

Open PowerShell:

PS> cd "H:\Shared drives\GitHub\Dryer-pi"

PS> git init
Initialized empty Git repository

PS> git add .
[Files added...]

PS> git commit -m "Initial commit"
[main (root-commit) abc123] Initial commit
 20 files changed, 2000 insertions(+)


┌─────────────────────────────────────────────────────────────┐
│  STEP 3: Connect to GitHub                                 │
└─────────────────────────────────────────────────────────────┘

PS> git remote add origin https://github.com/YOUR_USERNAME/dryer-pi.git
                                ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                Paste URL from Step 1!

PS> git branch -M main


┌─────────────────────────────────────────────────────────────┐
│  STEP 4: Push to GitHub                                    │
└─────────────────────────────────────────────────────────────┘

PS> git push -u origin main

Username: YOUR_USERNAME
Password: [Paste Personal Access Token]
          ^^^^^^^^^^^^^^^^^^^^^^^^
          NOT your GitHub password!
          
Enumerating objects: 25, done.
Counting objects: 100% (25/25), done.
Delta compression using up to 8 threads
Compressing objects: 100% (20/20), done.
Writing objects: 100% (25/25), 15.24 KiB | 1.52 MiB/s, done.
Total 25 (delta 2), reused 0 (delta 0), pack-reused 0
To https://github.com/YOUR_USERNAME/dryer-pi.git
 * [new branch]      main -> main

✅ Done! Your code is now on GitHub.


┌─────────────────────────────────────────────────────────────┐
│  GETTING A PERSONAL ACCESS TOKEN                           │
└─────────────────────────────────────────────────────────────┘

GitHub no longer accepts passwords for git operations.
You need a Personal Access Token (PAT):

1. Go to: https://github.com/settings/tokens

2. Click: [Generate new token] → [Generate new token (classic)]

3. Fill in:
   Note: "Dryer-Pi development"
   Expiration: 90 days (or your choice)
   
4. ✅ Check these scopes:
   [✓] repo (all sub-checkboxes)
   
5. Click: [Generate token]

6. 📋 COPY THE TOKEN! (You can't see it again)
   Example: ghp_1234567890abcdefghijklmnopqrstuvwxyz

7. Use this token as your password when git asks.

💡 TIP: Save token in password manager!


┌─────────────────────────────────────────────────────────────┐
│  ALTERNATIVE: Use GitHub CLI (Easier!)                     │
└─────────────────────────────────────────────────────────────┘

If you have GitHub CLI installed:

PS> gh auth login
[Follow prompts to authenticate]

PS> cd "H:\Shared drives\GitHub\Dryer-pi"

PS> gh repo create dryer-pi --public --source=. --push

✅ Done in one command! Repo created and code pushed.

Install GitHub CLI: https://cli.github.com/


┌─────────────────────────────────────────────────────────────┐
│  VERIFY IT WORKED                                          │
└─────────────────────────────────────────────────────────────┘

Open browser:
https://github.com/YOUR_USERNAME/dryer-pi

You should see all your files:
  📄 CMakeLists.txt
  📄 dryer-main.cpp
  📄 dryer-physics.cpp
  📄 README.md
  📄 setup.sh
  ... and more

✅ Success! Now move to Pi setup (Step 3 in GIT_WORKFLOW.md)


┌─────────────────────────────────────────────────────────────┐
│  COMMON ERRORS & FIXES                                     │
└─────────────────────────────────────────────────────────────┘

ERROR: "remote origin already exists"
FIX:   git remote remove origin
       [Then try again]

ERROR: "repository not found"  
FIX:   - Check URL is correct
       - Make sure repo exists on GitHub
       - Check you're logged in to correct account

ERROR: "Authentication failed"
FIX:   - Use Personal Access Token, NOT password
       - Create new token at github.com/settings/tokens
       - Make sure token has 'repo' scope

ERROR: "Permission denied"
FIX:   - Check token hasn't expired
       - Generate new token
       - Make sure you own the repository


┌─────────────────────────────────────────────────────────────┐
│  WHAT'S NEXT?                                              │
└─────────────────────────────────────────────────────────────┘

Your code is now on GitHub! 🎉

Next steps:

1. Flash SD card (see GIT_WORKFLOW.md Part 2)
2. SSH into Pi
3. Clone your repo:
   ssh dryer@dryer-pi.local
   git clone https://github.com/YOUR_USERNAME/dryer-pi.git
4. Run setup:
   cd dryer-pi
   sudo ./setup.sh
5. Reboot and enjoy!

Full guide: GIT_WORKFLOW.md
Quick ref: CHEATSHEET.md
```
