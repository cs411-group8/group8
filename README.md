# Using the Group 8 Git Repository
## Setting up SSH Keys

Github may prompt you to do this when you sign up, but if not you will need to generate an SSH key. On Linux (I recommend flip) you can use:

    ssh-keygen -t rsa -C "<youronid>@onid.orst.edu"

Note that you don't have to use your ONID address, anything at all will work (it doesn't even have to be real).

When you are prompted for a passphrase, you can either enter one (recommended) or simply press enter to not use one.

## Adding your SSH Key to your Github Account

Log into Github and go to (in the top right corner) "Account Settings" then select "SSH Public Keys" to add your key. The text of your public key (which isn't a secret, you can share it with anyone) is located in ~/.ssh/id_rsa.pub after you run the above command.

## Get Write Access to This Repository

Send me (Russell) your Github username and I'll grant you write access to this repository.

## Cloning the Repository

Note that you will first need to copy your private key (~/.ssh/id_rsa) to to whatever computer you want to use Github from. To clone the git repository run

    git clone git@github.com:cs411-group8/group8.git

And type 'yes' when prompted. This should take a few minutes, but probably not more than around 5, even on the crappy VM.
