# SoftwareDefinedRadio4JUCE

A framework to use JUCE for Software Defined Radio. Including a JUCE-style SDR hardware abstraction layer to interface SDRs from various companies, some RF specific DSP classes and OpenCL accelerated DSP classes that can be compiled for FPGAs.

Note that this project is in its initial phase, so significant API changes to the core functionality are likely at this time.

## Core components:

The framework consists some core components that can be used as building blocks to quickly set up an SDR application.

### Complex valued sample buffers
As complex values are common in the SDR world the framework comes with a unique set of sample buffer classes. Some functions use AVX2 instructions on Intel architectiure for speedup. As a main goal of this project is to evaluate the usability of OpenCL for DSP processing there are interface definitions for buffers that use OpenCL mapped memory.

### File exchange format

As storing complex valued sample buffers and matrices is a common task and there is no standard format for such RF related data, we created the .mcv (Matrix complex valued) file format. The framework contains classes for reading and writing whole files into a buffer as well as for continuously reading from or writing to a file. Furthermore MATLAB-compatible read and write functions come with the framework. 

### Hardware abstraction layer:

The abstract `SDRIOEngine` class defines a common interface for all kind of IO backends. For all tunable hardware SDRs, the `SDRIOHardwareEngine` furthermore defines an extended API. They can be used as base to implement a compatible IO backend. The `SDRIODeviceManager` class is used by the user to access an engine.
Currently the following engines are implemented:

- MCVEngine: A software backend, reading/writing mcv files
- UHDEngine: A backend that interfaces with Ettus UHD compatible devices. Checks at runtime if a current version of the UHD library is present on the system. It only uses a subset of the most common UHD functionalities and has no dependencies on the Ettus UHD sources. Currently in an experimentatl state.