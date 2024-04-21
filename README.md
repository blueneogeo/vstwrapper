# VST Wrapper

This plugin is meant to facilitate two-way VST communication from DAWs to controllers such as the Electra One.

To use it, instead of loading the original VST, you load this VST, and let it load and host your original VST. This VST then lets you take outgoing VST sysext and CC messages and route them to other channels. This lets you route these changes on the original VST to devices such as the Electra One.

## Getting started on MacOS

For now, only MacOS is supported. The project is based on [PampleJuce](https://github.com/sudara/pamplejuce/). See that for more information on setting up.

Setup CMake project:

```
cmake -B Builds -G Ninja
```

Perform a build:

```
cmake --build Builds --config Release
```

If you use Helix as an editor, Ninja will create a compile_commands.json in the Builds directory. If you update files or dependencies, build the code and copy compile_commands.json into the root of the project (where you find this README.md). This will let Helix know the paths of files.

Also, when in Helix, press backspace and then b to build the release version of the project.
