sudo apt install g++ make cmake curl mingw-w64 libfltk1.3-dev
make          # native Linux binary
make win      # gamelauncher.exe

I need a program that runs on windows and has the following requirements:
- It should be as nimble as possible
- It should have a GUI
- It should work on windows and linux
- It serves as a preconfiguration frontend for a game whose path is editable.
- When it opens, it lets the user choose one of different resolutions and one of different controllers. The user can add and remove resolutions and controllers.
- With those options chosen, the program copies over one version or the other of a file (or set of files) to the game's installation path.
- For all of this to work, the program probably needs its own folder to store the different versions, and some config file to save the state among runs.
