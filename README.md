README {#mainpage}
====

The documentation is generated by doxygen & hosted on github pages (See repository description).

# SUBDIRECTORIES
* errorcorrection (@subpage ec_readme)
    * error correction packet formats (@subpage ec_formats)
    * Thoughts on LDPC (@subpage ldpc_readme)
    * Thoughts on Aff3ct (@subpage ldpc_aff3ct)
    * ldpc_examples (@subpage ldpc_examples_readme)
        * my_project_aff3ct (@subpage my_project_with_aff3ct_readme)
            * ci (@subpage bootstrap_ci)
            * examples/bootstrap (@subpage bootstrap_example)
                * src (@subpage bootstrap_example_src)
* hardware (@subpage hardware)
* packetheaders (@subpage filespec)
* remotecrypto (@subpage remotecrypto)
* scripts (@subpage scripts)
* timestamp3 (@subpage timestamp3)
* usbtimetagdriver (@subpage usbtimetagdriver)
* misc_documentation
    * additional documentation/files that is not generated from the code
* tools
    * Miscellaneous tools that are useful, but unrelated to the execution of the stack and do not have a GitHub repository

# OTHER FILES / SUBDIRECTORIES
* docs: Contains documentation generated by [Doxygen](doxygen.nl/)
* Doxyfile: Doxygen config file
* run_doxygen.sh: Helper bash script to run doxygen on linux. This will generate this documentation for you.

# DISCLAIMER(S)
* Only the source code in the `errorcorrection` directory has been annotated for Doxygen. Your mileage may vary w.r.t. other components in terms of their doxygen documentation; best to refer to their source code.

# SETTING UP YOUR DEVELOPMENT ENVIRONMENT
* If you don't already have a code editor, I highly recommend [Visual Studio Code](https://code.visualstudio.com/) which can install extensions like [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) that automatically highlights errors in your code and provide autocomplete features such as IntelliSense and GUI debugging tools.
    * Visual Studio Code also has a [Doxygen Documentation Generator](https://marketplace.visualstudio.com/items?itemName=cschlosser.doxdocgen) extension.

# SETTING UP THE QCRYPTO STACK
## Languages
* Bash, C, C++

## Library Dependencies
* [fftw3](http://www.fftw.org/)

The stack is designed to be run on Unix platforms (e.g. Linux). For a guide on how to setup the stack, please refer to the jupyter notebook file in the repository.

# GIT & GITHUB
You may be familiar with GitHub as a website that hosts and stores your repositories. But what makes it really useful is its ability to help you do various other things, such as:
* [Allow you to make your own copy, after which you can open a Pull Request to merge your changes](https://help.github.com/en/enterprise/2.13/user/articles/fork-a-repo)
* [Allow you to make changes in the same repository without modifying the code for others using branches](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/about-branches)
* [Manage merges from other branches or even within the same branch](https://help.github.com/en/github/collaborating-with-issues-and-pull-requests/merging-a-pull-request)
* [Allow you to manage releases](https://help.github.com/en/github/administering-a-repository/managing-releases-in-a-repository)
* [Allow you to tag commits if they signify an important checkpoint / important changes were made](https://www.freecodecamp.org/news/git-tag-explained-how-to-add-remove/)

You don't have to use these features but they are good to know. For simplicity sake I would recommend using a GUI for Git such as [SourceTree](https://www.sourcetreeapp.com/).

# DOCUMENTING THIS PROJECT
This project remains a work in progress and there will no doubt be many other contributors. A vital portion of developing this project involves its documentation. 

## THIS SOUNDS LIKE A LOT OF WORK... WHY DO I NEED TO DOCUMENT?
* Document for users of the stack. Not everyone needs to open the components up and modify the code, but some new hands may come in and need to know how to integrate these components and use the stack. Documentation for these users makes their lives significantly easier. The difference between good and bad documentation can mean the difference between a few minutes of setup time to a few hours (or months) of setup time.
* Document for future developers of the stack. This allows future developers to understand why you do something the way you do it, and also understand how you do it so they can contribute in an effective way.
* Document for yourself. You won't remember every decision you make and you'll probably forget what a function does if you have to drop the project for another task.
* Good code is a joy to work with. Don't you feel happy that people can use your work without complaining about it?

## HOW DO I DOCUMENT?
If you are new to documenting projects, [here](https://nus-cs2103-ay1920s2.github.io/website/se-book-adapted/chapters/documentation.html) is a guide. It may sound like a lot of work, but once you're used to it, it really doesn't take that much more of your time. There are 3 areas to keep a note of when documenting the code:
1. First, the **code**. It doesn't matter how clean your commit messages are, or how comprehensive your additional documentation is if nobody can understand the code. Besides trying your best to keep the [code quality](https://nus-cs2103-ay1920s2.github.io/website/se-book-adapted/chapters/codeQuality.html) high, this project uses [Doxygen](doxygen.nl/), which basically generates documentation on your C, C++ files based on how you annotate it. Doxygen comes with a [manual](https://www.doxygen.nl/manual/index.html), and you can refer to the errorcorrection module to see how this annotation is done. Using Doxygen may sound troublesome at first, but the structure it provides also results in cleaner code in general.
    * Once you have finished annotating the code, you will want to generate the documentation using Doxygen, which helps you to generate documentation from your annotations which saves you time. 
        * If your computer doesn't have doxygen, you'll need to install Doxygen and its dependencies, [GraphViz](https://graphviz.org/) (for diagram purposes) and PlantUML (It has already been included in the tools folder and incorporated into the Doxyfile; just make sure you have Java on your computer. See [this](https://www.doxygen.nl/manual/commands.html#cmdstartuml)) (for UML diagram purposes). 
            * Once doxygen & its dependencies are installed, if you're running this on Linux, you can just execute the run_doxygen.sh bash script to generate the documentation. Otherwise, you can open the file and see the steps I used to generate the script so that you can execute similar steps on your OS.
            * The version of plantuml in the  tools  directory is `plantuml-jar-gplv2-1.2020.14`.
            * I use [this website](https://www.planttext.com) to preview my plantuml.
2. Besides annotating the code, the project also has READMEs and other documents which outline higher level details not covered in the code which is in [markdown](https://github.com/adam-p/markdown-here/wiki/Markdown-Cheatsheet) format. Each of these readmes are also included in the Doxygen output using headers (that's why they have the curly braces etc) to identify them, and they are referenced in the README in the root directory.
    * If you modify the markdown, you will also need to update the doxygen generated docs by running Doxygen.
    * The cool thing about this website that doxygen generates is that it can be automatically hosted using [GitHub pages](https://help.github.com/en/github/working-with-github-pages/configuring-a-publishing-source-for-your-github-pages-site). In this current setup, so long as changes were made to the the docs folder in the master branch (i.e. main branch) you should see the changes reflected.
3. Once all this is done, you'll want to commit and push the code to your repository; what message should you include with the commit? [This](https://chris.beams.io/posts/git-commit/) can help.

# GOOGLE TEST SUITE
This project aspires to test its components with GTS (unfortunately I didn't have time to write test cases). You'll have to download,
install and compile GTS locally on your computer (this can be done with `setup_gtests.sh`). Usage tutorial can be found
[here](https://notes.eatonphil.com/unit-testing-c-code-with-gtest.html) and [here](https://github.com/google/googletest/blob/master/googletest/docs/primer.md).

## Other testing solutions?
You may want to look into behavior-driven development.

# LICENSE
 Copyright (C) 2020 Matthew Lee, National University
                         of Singapore <crazoter@gmail.com>

 Copyright (C) 2005-2015 Christian Kurtsiefer, National University
                         of Singapore <christian.kurtsiefer@gmail.com>

 This source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU Public License as published 
 by the Free Software Foundation; either version 2 of the License,
 or (at your option) any later version.

 This source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 Please refer to the GNU Public License for more details.

 You should have received a copy of the GNU Public License along with
 this source code; if not, write to:
 Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
