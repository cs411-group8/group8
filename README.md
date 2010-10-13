# Setting up the Group 8 Git Repository
## Setting up SSH Keys

Github may prompt you to do this when you sign up, but if not you will need to
generate an SSH key. On Linux (I recommend flip) you can use:

    ssh-keygen -t rsa -C "<youronid>@onid.orst.edu"

Note that you don't have to use your ONID address, anything at all will work
(it doesn't even have to be real).

When you are prompted for a passphrase, you can either enter one (recommended)
or simply press enter to not use one.

## Adding your SSH Key to your Github Account

Log into Github and go to (in the top right corner) "Account Settings" then
select "SSH Public Keys" to add your key. The text of your public key (which
isn't a secret, you can share it with anyone) is located in ~/.ssh/id_rsa.pub
after you run the above command.

## Get Write Access to This Repository

Send me (Russell) your Github username and I'll grant you write access to this
repository.

## Cloning the Repository

Note that you will first need to copy your private key (~/.ssh/id_rsa) to to
whatever computer you want to use Github from. To clone the git repository run

    git clone git@github.com:cs411-group8/group8.git

And type 'yes' when prompted. This should take a few minutes, but probably not
more than around 5, even on the crappy VM.

## Configuring Your Name and Email

You should configure your name and email so your commit messages look nice.

    git config user.name "Joe Student"
    git config user.email "studenj@onid.orst.edu"

You can also do

    git config --global user.name "Joe Student"
    git config --global user.email "studenj@onid.orst.edu"

If you want to make these settings apply for every repository that you work
with (as your user on the computer in question).

# Working With the Group 8 Git Repository

I'll go into more detail below, but the really short version is something like
this

    git pull              # Download the latest changes from the server
    vim foo/bar.txt       # Make some changes
    git add foo/bar.txt   # Tell git about the changes
    git commit            # Commit the changes
    git pull              # Check for updates on the server
    git push              # Push your changes to the server
    vim foo/baz.txt       # Repeat


## Committing Changes

After you've modified some files you'll want to commit your changes. To see
what you've done, run 'git status'.

    # On branch master
    # Changed but not updated:
    #   (use "git add <file>..." to update what will be committed)
    #   (use "git checkout -- <file>..." to discard changes in working directory)
    #
    #       modified:   README.md
    #
    # Untracked files:
    #   (use "git add <file>..." to include in what will be committed)
    #
    #       new_file.txt
    no changes added to commit (use "git add" and/or "git commit -a")

Notice that git differentiates between 'tracked' and 'untracked' files. If you
only wanted to commit 'new_file.txt' you could run 'git add new_file.txt', in
which case 'git status' would now show

    # On branch master
    # Changes to be committed:
    #   (use "git reset HEAD <file>..." to unstage)
    #
    #       new file:   new_file.txt
    #
    # Changed but not updated:
    #   (use "git add <file>..." to update what will be committed)
    #   (use "git checkout -- <file>..." to discard changes in working directory)
    #
    #       modified:   README.md
    #

If you also wanted to commit README.md you could run 'git add README.md' or
'git add .' to add all files in the current directory. The 'git status' output
does a pretty good job of explaining how to modify the status of files, but if
you have questions there is lots of info on the internet, or ask me.

Once you are ready to commit (everything listed under "Changes to be
committed"), simply type "git commit". You can use the '-m' option to specify
a commit message from the command line, otherwise it will open your $EDITOR
for you to enter a message.

## Pushing Changes

Unlike SVN, in git when you commit a change all you have done is created a
commit for it locally. That is, you still need to push your change to the
server in order for anyone else to see it.

The first thing you'll need to do is make sure no one else has committed any
changes to the server since your last commit. You can do this with 'git pull',
which will pull changes from the server and merge your changes into the
history. Another option is to use 'git pull --rebase', which will roll back
any changes you made locally, download the latest revision from the server,
then play your changes back on top of that.

Whichever way you choose to do it, if both you and another user have modified
the same line in the same file, there will be a merge conflict and git will
prompt you to fix it. Basically you have to open the file and look for a line
that has a bunch of '<<<<' characters. The conflict might look something like
this.

    <<<<<<< HEAD:file.txt
    Hello world
    =======
    Goodbye
    >>>>>>> 77976da35a11db4580b80ae27e8d65caf5208086:file.txt

This means that one person (you, if I remember correctly) wrote 'Hello world'
on this line, while another person wrote 'Goodbye' on this line. Fix it to
look how you would like, so for example you might just remove the markers
and the other person's line like

    Hello world

At this point you will need to 'git add' this file (if you run 'git status'
it should tell you this) then, depending on whether you used 'git pull' or
'git pull --rebase' you either 'git commit' (which creates a "merge commit")
or 'git rebase --continue' (which modifies your original commit when it gets
played back on top).

Finally you can push:

    git push

Fancy.
