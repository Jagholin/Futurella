Futurella
=========

A game of fast delivery

- OpenSceneGraph 3
- CMake
- FMod
- boost::asio
- CEGUI
- BulletPhysics 2.8.2


How to build
==========

Windows:

1. You will need CMake, an already built OpenSceneGraph 3(perhaps from sources), CEGUI, and VisualStudio 2013+.

2. Edit _configure.bat to contain paths to the library directories on your computer

3. Run _start-cmake-gui.bat to open CMake GUI, set source and build directories, and press Configure and then Generate.

4. Edit _open-project.bat to contain path to your solution file, that was produced in the last step(3)

5. Run _open-project.bat to start Visual Studio.

6. Select "Futurella" as StartUp project

7. go to this project's properties, and on the "debug" tab change Working Directory to $(TargetDir) for all configurations(Debug, Release)

8. Press Ctrl+F5 to compile, link and run the project.



Linux:

1. As linux is considered OS for profis, i don't feel the need to lecture you on how to use all the tools. Even if i knew how to:)

2. If you have fixes/suggestions, just contact me.



MacOS:

1. We don't support this system. It may well run, but we won't make any promises.


Also
=========

In March/April we'll drop OpenSceneGraph in favor of a more appropriate Framework, it will be perhaps ~~Ogre3d~~ Magnum.
