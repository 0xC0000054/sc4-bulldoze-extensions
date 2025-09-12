# sc4-bulldoze-extensions

A DLL Plugin for SimCity 4 that extends the bulldoze tool.   

The plugin can be downloaded from the Releases tab: https://github.com/0xC0000054/sc4-bulldoze-extensions/releases

## Features

### Flora Bulldoze Mode

This mode is activated in the city view using a _Control + B_ shortcut, this can be done with or without the bulldoze tool active.
When the flora bulldoze mode is active, the bulldoze tool will only affect objects the game considers flora.

### Network Bulldoze Mode

This mode is activated in the city view using a _Shift + B_ shortcut, this can be done with or without the bulldoze tool active.
When the network bulldoze mode is active, the bulldoze tool will only affect transportation networks (excluding power lines and water pipes).

## Configuration Options

### Bulldoze Extensions Tuning Exemplar

This exemplar defines the following properties to control the highlight color used for the demolished area.
All 3 properties consist of 4 Float32 values that define the colors in RGBA order.
Converting a color channel to/from float can be done by dividing/multiplying the value be 255, note that the resulting value may be +/- 1 due to rounding.

SC4 default green highlight is RGBA Float 0.0, 0.7, 0.2, 0.3 (RGBA 0, 179, 51, 77).

| Property Name | Property ID | Description |
|---------------|-------------|-------------|
| Bulldoze Extensions Normal Highlight Color | 0x8FD94ED0 | The highlight color used when not in the flora or network modes. Defaults to blue (RGBA 77, 153, 217, 128). |
| Bulldoze Extensions Flora Highlight Color | 0x8FD94ED0 | The highlight color used when not in the flora mode. Defaults to green (RGBA 97, 176, 97, 128). |
| Bulldoze Extensions Network Highlight Color | 0x8FD94ED0 | The highlight color used when not in the flora or network modes. Defaults to orange (RGBA 250, 153, 51, 128). |

## System Requirements

* SimCity 4 version 641
* Windows 10 or later

The plugin may work on Windows 7 or later, but I do not have the ability to test that.

## Installation

1. Close SimCity 4.
2. Copy `SC4BulldozeExtensions.dll` and `BulldozeExtensions.dat` into the top-level of the Plugins folder in the SimCity 4 installation directory or Documents/SimCity 4 directory.
3. Start SimCity 4.

## Troubleshooting

The plugin should write a `SC4BulldozeExtensions.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the GNU Lesser General Public License version 3.0.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll/tree/master) Located in the vendor folder, MIT License.    
[Windows Implementation Library](https://github.com/microsoft/wil) - MIT License    
[SC4Fix](https://github.com/nsgomez/sc4fix) - MIT License    

# Source Code

## Prerequisites

* Visual Studio 2022
* `git submodule update --init`
* [VCPkg](https://github.com/microsoft/vcpkg) with the Visual Studio integration

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in full screen with the following command line:    
`-intro:off -CPUcount:1 -w -CustomResolution:enabled -r1920x1080x32`

You may need to adjust the window resolution for your primary screen.
