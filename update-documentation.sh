# Helper script to update the documentation on the gh-pages branch as well

git checkout gh-pages
# Make sure to modify the branch name here if you use a different branch
git rebase branch-ecd2-enhancements3
git push gh-pages
./run_doxygen.sh
git add .
git commit -m "Update documentation"
# You may need auth here
git push
# Use the correct branch name here if needed
git checkout branch-ecd2-enhancements3
