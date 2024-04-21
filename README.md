# VST Wrapper

This plugin is meant to facilitate two-way VST communication from DAWs to controllers such as the Electra One.

To use it, instead of loading the original VST, you load this VST, and let it load and host your original VST. This VST then lets you take outgoing VST sysext and CC messages and route them to other channels. This lets you route these changes on the original VST to devices such as the Electra One.

## Getting started on MacOS

For now, only MacOS is supported. The project is based on [PampleJuce](https://github.com/sudara/pamplejuce/). See that for more information on setting up.
