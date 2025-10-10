# Ain INDIGO Imager - Users Guide
*(Ain Imager v.2.3)*

Revision: 21.04.2025 (draft)

Author: **Rumen G.Bogdanovski**

e-mail: *rumenastro@gmail.com*

## Table of Contents
1. [Introduction](#introduction)
1. [Main Window](#main-window)
1. [General Concepts](#general-concepts)
1. [Connecting to INDIGO services](#connecting-to-indigo-services)
1. [Image capture](#image-capture)
1. [Sequences](#sequences)
1. [Focusing](#focusing)
1. [Guiding](#guiding)
1. [Telescope control](#telescope-control)
1. [Plate solving](#plate-solving)
1. [Accessing advanced device and agent settings](#accessing-advanced-device-and-agent-settings)
1. [Managing service configurations](#managing-service-configurations)


## Introduction
*Ain INDIGO Imager* is a free and open-source astronomical image acquisition application distributed under the [INDIGO Astronomy open-source license](https://github.com/indigo-astronomy/indigo_imager/blob/master/LICENSE.md). *Ain* is designed to be easy to use and lightweight. It requires a running [INDIGO Server](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/INDIGO_SERVER_AND_DRIVERS_GUIDE.md).

*Ain* is supported on Linux and Windows operating systems and is available for download on the [INDIGO Astronomy](https://www.indigo-astronomy.org/downloads.html) website.

*Ain* is designed to be simple with very little business logic embedded; all the hard work is done by the INDIGO agents. It is essentially a user interface for the INDIGO agents. Because of that, upon connecting, *Ain* will automatically load any agents it needs to operate.


## Main Window

The main window is divided into three main areas as seen in the picture below:

![](images_v2/capture_main_color.png)

Almost all widgets (buttons, drop-down menus, spin boxes, etc.) have tool-tips with information on what the widget does. In the case of numeric values, they also indicate the range of valid input.

### Control Area
This is the upper left area consisting of several tabs and sub-tabs. The control area is where all configuration is done and all processes are controlled, such as guiding, taking exposures, etc. It will be described in detail in the following chapters.

### Image Area
This is the upper right area of the main window. In this area, the most recently captured image will be displayed. It is context-dependent as follows:

* In the **Capture**, **Focus**, and **Telescope** tabs, the last image from the main imaging camera will be displayed. Several overlays can be turned on or off such as image statistics (controlled by **Settings -> Show image statistics**) and image center mark (**Settings -> Show image center**).

* In the **Guider** tab, the most recent image from the guiding camera is displayed and overlays for the selected guide stars and the image’s reference point are shown. This is described in detail in a later chapter.

* In the **Solver** tab, the image currently being solved is displayed.

When the mouse cursor is over the image, the current pixel value will be displayed in the upper right along with the mouse coordinates and the zoom level. Next to this are several buttons for controlling zooming and stretching.

If the **Solver** or **Telescope** tab is active and the current image has been solved, the Right Ascension and Declination of the point under the mouse cursor are shown instead of the pixel coordinates. In this case, **Right-Clicking** on the image will copy the coordinates under the mouse into the **Telescope** tab. Pushing the **Goto** button will then slew the telescope to center on these coordinates (use **Control + Right-Click** to copy the coordinates and slew the telescope as a single action).

The mouse wheel zooms in or out of the image and **Left-Click + Drag** will pan the image.

When the image area is active, several shortcut keys can be used for zooming:
* **'+'** zoom in
* **'-'** zoom out
* **'1'** zoom 1:1
* **'f'** zoom to fit

### Log Area
The log area is the bottom part of the main window. In this area, all messages from the INDIGO framework will be displayed, preceded by a timestamp. There are three types of messages: *Information* - displayed in white, *Error* - displayed in red, and *Warning* - displayed in yellow. *Ain* does not display errors or warnings in dialog boxes; all errors, warnings, and messages are displayed in this log. In addition, each message can be accompanied by an audible notification. Audible notifications are enabled by **Settings -> Play sound notifications**.

## General Concepts

### Data Storage
By default, Ain will store the files in a subdirectory of your home directory called "ain_data". Frames obtained on different dates will be saved in different subdirectories with the format "YYYY-MM-DD". The output directory changes automatically at noon, to keep the data from the same night in the same directory. The data directory can be changed from **File -> Select Data Directory**.

### Widget Color Coding
*Ain Imager* uses colors to represent the states of the operations. Related widgets will be decorated differently depending on the status. **Default** color (or **green** in some cases) means that the operation is idle or finished successfully. **Red** means the operation failed or was canceled by the user. **Yellow** means the operation is in progress.

For example:

* Aborted exposure, **Expose** button in **red**:

	![](images_v2/exposure_aborted.png)

* Exposure in progress, **Expose** button in **yellow**:

	![](images_v2/exposure_busy.png)

* Exposure completed successfully, **Expose** button in default color:

	![](images_v2/exposure_ok.png)

## Connecting to INDIGO Services
*Ain* will automatically discover all INDIGO services available on the network. Depending on **Settings -> Auto connect new services**, it will connect or not to the newly discovered services.

Services can be managed from **File -> Available Services** as shown below.

![](images_v2/available_services.png)

All available services will be listed. Use the checkboxes to connect or disconnect from a service. The connection status of the services will be restored when *Ain* is restarted, provided the service is still available. In the example above, the service "indigosky" is connected and "vega" and "indigo_test" are not.

The auto-discovered services will be listed with the "Bonjour" icon in front of their name ("indigosky" and "vega") and cannot be removed from the list. They will disappear when the server shuts down and reappear when it is back online.

Services that are not discoverable (not announced on the network or that are on a different network, in a remote observatory for example) can be manually added by the user. The service should be specified in the form **name\@host.domain:port** or **name\@ip_address:port** in the text field below the service list. If **port** is not specified, the default INDIGO port (7624) is assumed. Also, **name** has only one purpose, to give some meaningful name to the service and has nothing to do with the remote service. It can be any text string. If not specified, **host** will be used as a service name. Such services are displayed with a blue planet icon ("indigo_test") and can be removed manually.

## Image capture
As mentioned above, *Ain Imager* uses agents to operate and the top drop-down menu is for agent selection. In the **Capture** tab, all available *Imager Agents* from all connected services will be listed. Depending on **Settings -> Use host suffix**, it will show or not show the service name as a suffix (takes effect after reconnect). *Ain* can use only one imager agent at a time. Multi-agent support will come in the future.

![](images_v2/capture_main.png)

##### Camera
All cameras available to the selected *Imager Agent* will be listed in the drop-down. The selected camera will be used for image acquisition.

##### Wheel
All filter wheels available to the selected *Imager Agent* will be listed in the drop-down. The user should select the one attached to the selected camera.

##### Frame
All frame resolutions and pixel depths supported by the selected camera will be listed in the drop-down along with the frame types such as "Dark", "Light", "Flat", etc.

##### Exposure
Exposure time (in seconds) when the **Expose** or **Preview** button is pressed to start the capture of one or more images.

##### Delay
The delay (in seconds) between the exposures taken in a batch.

##### No Frames (Number of Frames)
How many frames should be taken in the batch exposure. Use -1 for an unlimited number of frames. In this case, the batch exposure will finish when the **Abort** button is pressed.

##### Filter
All available filters in the selected filter wheel will be listed in the drop-down. The selected filter is the one currently being used.

##### Object
The name of the object being photographed should be entered here. It is used as a prefix for the saved file name and as an object name in the FITS header. If there is no name specified and **Settings -> Save noname images** is checked, the "noname" string will be used as the file name prefix, otherwise, the images will not be saved and a warning will be printed in the log.

##### Cooler
If the selected camera can report the sensor temperature, the current temperature will be shown. If the camera supports cooling, it can be enabled and disabled here along with setting the target temperature. If cooling, the cooler power will be displayed.

### Image Tab
![](images_v2/capture_image.png)

##### Preview Exposure
The time (in seconds) used to get a preview frame with the **Preview** button.

##### Image Format
The image file formats supported by the camera driver will be listed here. They can be different for different cameras. The selected format will be used as a storage format for the saved images. **Raw data** format is a special case. It is supposed to be used by INDIGO internally, this is why batches and sequences will not save images taken as **Raw data** (as of Ain Imager version 0.99).

##### Region of Interest
The region of interest (ROI) can be configured by specifying **X** and **Y** of the top left corner and **Width** and **Height** of the sub-frame.

### Dithering Tab
INDIGO can dither between the frames and it is configured in this tab.

![](images_v2/capture_dithering.png)

##### Enable Dithering
Enables and disables dithering.

##### Dithering Strategy
The dropdown menu lists the available dithering strategies to choose from.

##### Amount
This value, in pixels, specifies the maximum number of pixels the frame should be shifted during dithering. The actual shift is a random value and this specifies the upper limit.

##### Settle Timeout
The settle timeout is in seconds. It specifies how much time to wait for the guiding to settle after dithering. If it has not settled before this timeout is up, a warning is issued and the imager agent will proceed with the next exposure.

##### Skip Frames
Specifies how many frames to skip between dithering (0 means to dither after each frame).

### Camera Tab
In this tab, camera-specific parameters can be set: gain, offset, and binning.

![](images_v2/capture_camera.png)

### Remote Images Tab
INDIGO services can work in the so-called "clientless" or "headless" mode. This means that the server can operate autonomously. To achieve that, the client must connect, configure the service to perform some specific task, start it, and disconnect. The server will store the data locally and when the client connects again it can download the acquired data. This mode is configured in this tab.

 ![](images_v2/capture_remoteimages.png)

To enable the server to store the captured frames locally, check **Save image copies on the server**. If one does not want downloaded images deleted from the server after downloading, check **Keep downloaded images on server** otherwise images will be deleted once downloaded.

If the server is configured to keep the downloaded images they can still be removed when no longer needed. This is achieved by unchecking **Keep downloaded images on server** and pressing **Server cleanup**. This will remove any images that have already been downloaded. Those not yet downloaded are kept. This is useful when the images should be downloaded to several locations and removed once downloaded everywhere.

## Sequences

Sequences are implemented in Ecma Script using the *INDIGO Scripting Agent* and are executed on the selected *Scripting Agent*. The user can add predefined actions to be executed. The sequence shown below will set up the selected gear, enable meridian flip, set the object name, calibrate and start guiding, and then take R, G, and B exposures in batches of 2 in a loop that will repeat 100 times. Then it will stop guiding, park the mount, etc.

![](images_v2/sequence_main.png)

### Editing Sequence
Each sequence consists of actions, which will be executed sequentially. Each action is described by zero or several properties like: Filter, exposure time, frame type, etc.

#### Sequence Name
The sequence name should be specified in the text field in the sequence view.

#### Add, Remove, Move Up, Move Down and Update Actions
To add a new Action to the sequence, the user should press the right mouse button at the position where it should be placed. A thin line (**Drop indicator**) will show the landing position and a menu will appear with all available actions:

![](images_v2/sequence_add.png)

- **Capture**
  - **Set Object Name** - Sets the object name used as the image filename prefix and OBJECT keyword in the FITS header.
  - **Capture Batch** - Captures a batch of images with the specified count and exposure time in seconds.
  - **Enable Camera Cooler** - Enables the camera cooler and sets the temperature to the specified value in degrees Celsius.
  - **Disable Camera Cooler** - Disables the camera cooler.
  - **Select Frame Type** - Selects the frame type from the available options.
  - **Select Camera Mode** - Selects the camera mode from the available options.
  - **Select Image Format** - Selects the image format from the available options.
  - **Set Camera Gain** - Sets the camera gain to the specified value.
  - **Set Camera Offset** - Sets the camera offset to the specified value.
  - **Set Directory (on server)** - Sets the directory on the server to the specified path.

- **Filter Wheel**
  - **Select Filter** - Selects a filter from the available options.

- **Focuser**
  - **Focus** - Focuses the camera with the specified exposure time in seconds.
  - **Focus (continue on failure)** - Focuses the camera with the specified exposure time in seconds and continues on failure.
  - **Set Focuser Position** - Sets the focuser position to the specified value.
  - **Clear Focus Selection** - Clears the focus selection.

- **Guider**
  - **Calibrate Guiding** - Calibrates the guiding with the specified exposure time in seconds.
  - **Start Guiding** - Starts guiding with the specified exposure time in seconds.
  - **Stop Guiding** - Stops guiding.
  - **Enable Dithering** - Enables dithering with the specified amount in pixels, time limit in seconds, and skip frames.
  - **Disable Dithering** - Disables dithering.
  - **Clear Guider Selection** - Clears the guider selection.

- **Mount**
  - **Slew** - Slews the mount to the specified RA and Dec coordinates.
  - **Precise Goto** - Performs a precise goto with the specified exposure time in seconds, RA, and Dec coordinates.
  - **Sync Center** - Syncs the mount with the solved frame and centers to the target RA and Dec.
  - **Enable Meridian Flip** - Enables meridian flip with the option to use a solver for precise centering with the specified offset from meridian in hours.
  - **Disable Meridian Flip** - Disables meridian flip.
  - **Park Mount** - Parks the mount.
  - **Home Mount** - Homes the mount.
  - **Unpark Mount** - Unparks the mount.

- **Rotator**
  - **Set Rotator Angle** - Sets the rotator angle to the specified value in degrees.

- **Flow Control**
  - **Repeat** - Repeats a set of actions for a specified number of iterations.
  - **Continue on Failure** - If an operation in the sequence fails, the execution will continue with the next action in the sequence.
  - **Abort on Failure** - If an operation in the sequence fails, the entire sequence will be aborted. This is the default behavior.
  - **Recover on Failure** - If an operation in the sequence fails, the execution will continue from the next defined **Recovery Point**.
  - **Recovery Point** - Marks a point in the sequence to resume execution if **Recover on Failure** execution policy is set. It has no effect with **Abort on Failure** and **Continue on Failure**

- **Devices**
  - **Select Imager Camera** - Selects an imager camera from the available options.
  - **Select Filter Wheel** - Selects a filter wheel from the available options.
  - **Select Focuser** - Selects a focuser from the available options.
  - **Select Guider Camera** - Selects a guider camera from the available options.
  - **Select Guider** - Selects a guider from the available options.
  - **Select Mount** - Selects a mount from the available options.
  - **Select GPS** - Selects a GPS device from the available options.
  - **Select Rotator** - Selects a rotator from the available options.

- **Misc**
  - **Wait** - Suspends the sequence for a specified time in seconds.
  - **Wait Until** - Suspend sequence execution until given date and time in UTC.
  - **Wait for GPS** - Waits for the GPS to acquire a signal.
  - **Send Message** - Sends a message with the specified content.
  - **Set FITS Header** - add/change keyword and value in the FITS header.
  - **Remove FITS Headr** - remove keyword from FITS header.
  - **Load Driver** - Loads a driver with the specified name.
  - **Unload Driver** - Unloads a driver with the specified name.
  - **Enable Verbose Logging** - Log more messages from the sequencer (default).
  - **Disable Verbose Logging** - Log less messages from the sequencer.
  - **Load Config** - Loads a configuration with the specified name.

Once the action is placed, the user should configure it accordingly. 

Actions can be freely dragged and dropped. This way the actions can be rearranged. As shown below:

![](images_v2/sequence_edit.png)

In order to remove actions, press the **(x)** button on the right side of the action widget.

#### Enable and Disable Execution of Specific Actions

You can enable or disable specific actions in a sequence by clicking on the status LED icon. When an action is disabled, the icon will change to indicate its disabled state. Disabled actions are visually represented as shown below:

![](images_v2/disabled_action.png)

- Disabling **Repeat** Action: If a **Repeat** action is disabled, all actions nested within it will also be disabled automatically.
- Re-enabling Nested Actions: If you enable a single nested action within a disabled **Repeat** action, the **Repeat** action itself will also be enabled.

This feature allows you to selectively include or exclude actions from execution without permanently removing them from the sequence.

#### Sequence Flow Control

There are three sequence execution policies: **Abort on Failure**, **Continue on Failure**, and **Recover on Failure**. Each policy takes effect for the actions following it in the sequence. Policies can be changed multiple times within a single sequence.

While the first two policies are self-explanatory (as described above), **Recover on Failure** requires more attention. With this policy, if an action fails, execution will continue from the next defined **Recovery Point**. Multiple recovery points can be defined within a single sequence.

Here is an example of using execution policies:

![](images_v2/failed_sequence.png)

In this example, we:
1. Select and configure the camera, filter wheel, and mount
2. Slew the mount to the target
3. Start a loop to take a series of exposures with B and V filters
4. Park the mount and unselect it
5. Unselect the filter wheel
6. Stop the camera cooler and unselect the camera

The sequence fails because filter "B" doesn't exist in the selected filter wheel. According to the policy set at the beginning, the sequence should recover on failure. We defined a recovery point at the end of the sequence where we shut down the equipment. At this point, we change the policy to **Continue on Failure** because we want to complete the entire shutdown section regardless of whether some actions fail.

#### Download Sequence from the Agent, Load from File and Save to File

The already loaded sequence can be downloaded from the selected *Scripting Agent* by clicking the download button (left button):

![](images_v2/sequence_download_load_save.png)

The sequence can be loaded from a file by clicking the folder button (the middle one) and choosing the sequence file.

The current sequence can be saved to a file by pushing the disk button (the right one) and providing a file name. The "*.js*" extension will be automatically added.

The sequence being edited or loaded will be uploaded to the *Scripting Agent* only when the sequence is started. Before that, all the changes will remain in the client only. Because of that, if the user changes the sequence while it is running the changes will not take effect.

**NOTE:** Loading a sequence from a file or downloading it from the agent will replace the current sequence and unsaved changes will be lost.

### Start, Pause, Reset, Abort and sequence progress monitoring

The current sequence can be started with the **Run** button. This will also upload the current sequence to the *Scripting Agent* as described above.

**Pause** will wait for the current action or exposure to complete and will pause the execution of the sequence before the next action begins. Actions are considered atomic operations from the sequencer's point of view and cannot be paused in the middle of their execution. The only exception is the **Capture Batch** action, which can be paused between individual exposures within the batch.

The **Abort** button will stop the current sequence. The completed actions will be decorated with green while the aborted one will be decorated red. 

**Reset** will clear the sequence state and clear the progress.

Clicking **Run** after abort will start from the beginning.

![](images_v2/sequence_progress.png)

Progress can be monitored using the three progress bars and the operation indicator as shown above. The first progress bar shows the elapsed time of each exposure. The second one shows the completed exposures in the current batch. The third one shows the sequence completion in percentages. Please note that the percentages represent the completed actions and are not related to time.

**Meridian Flip** will show if the meridian flip is enabled or disabled and if enabled it will give information on when it will happen as an offset from the transit time.

**Total Exposure** shows the total exposure time of all targets in the current sequence. It will be automatically calculated when the sequence is executed.

The **[&Sigma;]** button will update the total exposure time before the sequence is executed.

**NOTE:** Images aquired with a sequence are always saved on server.

## Focusing
Focusing is a feature of the *Imager Agent* and it works with the selected imager agent.

![](images_v2/focus_main.png)

##### Focuser
All focuser devices available to the selected *Imager Agent* will be listed here. The correct one should be selected.

##### Absolute Position
The current focuser position will be displayed here. Entering a new value and pushing the **[>]** button will move the focuser to the new position. It can be used to assist the focusing by roughly moving to focus or retracting the focuser at the end of the imaging session.

##### Relative Move
Relative move will help achieve precise focus manually. The value represents how many steps, relative to the current position, the focuser should move when the **[>>]** (focus out) or **[<<]** (focus in) button is pushed.

##### Reference T (Temperature)
The current temperature will be shown if the focuser has a temperature sensor. If **Auto compensation** is checked, the focus will be automatically corrected with the temperature change if the compensation factor is set in the **Misc** tab. This value must be determined by the user since it is setup dependent.

### Statistics Tab
![](images_v2/focus_statistics.png)

Essential focusing statistics along with a graphical representation will be displayed during manual or automatic focusing. The information displayed depends on the focus estimator being used. For the "Peak/HFD" estimator, half flux diameter (HFD) or full width half maximum (FWHM) will be displayed on the graph (configured with **Settings -> Peak/HFD Focuser Graph**). For "RMS Contrast", the contrast will be shown.

### Settings Tab
![](images_v2/focus_settings.png)

##### Exposure Time
Exposure time in seconds used during the focusing process.

##### Focus Mode
The focus mode can be "Manual" or "Auto".

##### Focus Estimators
Currently, the *Imager Agent* supports two focus estimators: "Peak/HFD" and "RMS Contrast".

##### Star Selection X/Y
The coordinates of the star that will be used for "Peak/HFD" focusing should be entered here. **Right-Click** on the image will load the coordinates of the cursor here and will move the selection overlay (a green square).

##### Selection Radius
The radius, in pixels, of the aperture used to estimate FWHM and HFD.

##### Autofocus Settings
Auto focus configuration is described in the [INDIGO Imager Agent - Autofocus Tuning Guide](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/IMAGING_AF_TUNING.md).

### Misc (Miscellaneous) Tab
![](images_v2/focus_misc.png)

##### Save Bandwidth
To transfer less data through the network, sub-frames with sizes of 10 or 20 radii centered around the selection can be used. This is applicable only for the "Peak/HFD" estimator.

##### Focus Compensation
This is the temperature compensation factor, in steps per degree Celsius. It specifies how many steps will be applied when **Auto compensation** is ON with each degree change of the ambient temperature.

##### On Focus Failed (Peak/HFD)
This defines what to do if the autofocus procedure fails. If **Return to the initial position** is checked, it will return to the starting position; otherwise, it will just stop. This applies to the "Peak/HFD" estimator only. With "RMS Contrast," it will always stop.

##### Invert In and Out Motion
If the focuser is inverted and retracts on focus out motion, the motion should be inverted. This drop-down menu will be active if the selected focuser supports motion inversion.

## Guiding
*Ain* uses the service *Guider Agent* for guiding. The top drop-down menu has a list of all available guider agents, and the appropriate one should be selected.

![](images_v2/guider_main.png)

##### Camera
All cameras available to the selected *Guider Agent* will be listed here. The appropriate one should be selected.

##### Guider
All guider devices available to the selected *Guider Agent* will be listed here. The appropriate one should be selected. They can be part of the mount or the ST4 port of the guiding camera.

##### Buttons
- **Preview** button will start a loop of exposures with the guider camera.
- **Calibrate** button will start the calibration process during which the guiding parameters will be determined automatically.
- **Guide** button starts the guiding process.
- **Stop** button will stop each of the above processes.

### Settings Tab
![](images_v2/guider_settings.png)

##### Exposure
Exposure time and delay between exposures can be specified in this section. Both are in seconds.

##### Guiding
*Guider Agent* provides several algorithms for drift detection and several options for guiding in declination. Both can be selected here.

In "Selection" guiding, the primary star can be selected by **Right-Clicking** on the star or by entering the coordinates in the **Star Selection X/Y** fields. The radius of the circle where the star can drift between two consecutive frames is specified in the **Selection radius** field. For multi-star "Selection" guiding, the number of stars to be used can be specified in the **Star count** field.

Only the first star can be selected manually or automatically. All other stars can only be auto-selected. The list of the selected stars can be cleared by pushing the **Clear star selection** button. This will force the *Guider Agent* to make a new selection when any process is started.

If the declination guiding runs away after a meridian flip, **Reverse Dec speed after meridian flip** should be checked. Some mounts track their "side of pier" state and automatically reverse the direction of the declination motor after a meridian flip. Other mounts do not do this. There is no way for INDIGO to know this in advance. This is why INDIGO needs this to be specified.

In "Donuts" guiding, the whole frame is used to detect drift, but some cameras have an unusable border around the frame. This can interfere with the guiding, and this area can be excluded by specifying **Edge Clipping**.

### Advanced Tab
![](images_v2/guider_advanced.png)

*Guider Agent* configuration is described in [INDIGO Guider Agent - PI Controller Tuning](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/GUIDING_PI_CONTROLLER_TUNING.md) and in [Guider Agent README](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_guider/README.md).

Calibration parameters will be automatically computed during the "calibration" process, but they can be fine-tuned manually.

### Misc (Miscellaneous) Tab
![](images_v2/guider_misc.png)

##### Save Bandwidth
There are two options to reduce network traffic. As the guiding is done on the server, *Ain Imager* does not need full-resolution raw frames. It just needs basic images for visualization. This is why the user can choose to download JPEG images, which can be more than 10x smaller, and image sub-frames.

##### Camera Settings
Frame format, gain, and offset of the guiding camera can be specified in this section.

##### Guider Scope Profile
The focal length of the guider scope should be entered in order to enable the drift to be displayed in arc seconds. Otherwise, **Settings -> Guider Graph -> RA / Dec Drift (arcsec)** will have no effect, and the statistics and graph views will be in pixels.

## Telescope control
*Ain* uses the service *Mount Agent* to control mounts. The top drop-down menu has a list of all available mount agents and the appropriate one should be selected.

If the displayed image is solved a **Right-Click** on it will copy the coordinates under the mouse cursor in the **RA / Dec input** boxes and pushing **Goto** button will slew the telescope to these coordinates. **Control + Right-Click** will copy and slew the telescope in one action.

![](images_v2/telescope_main_solved.png)

##### Mount
All mounts available to the selected *Guider Agent* will be listed here. The appropriate one should be selected.

##### Az / Alt
Displays current target Azimuth and Altitude and uses color coding to give a visual clue of the target suitability to be observed:
* **Green** - the target is high above the horizon and is good to observe.
* **Yellow** - the target is low above the horizon.
* **Red** - the target is very low or below the horizon.

##### RA / Dec Input
In these fields the target Right ascension and Declination should be entered. They will be used for goto if **Goto** is button is pushed or to synchronize the mount to these coordinates if **Sync** is button is pushed.

### Main Tab
![](images_v2/telescope_main.png)

##### N S W E buttons and slew rates
The mount can be moved by pushing **N S W E** buttons. The speed is selected from four available presets. The slowest is **Guide rate** and the fastest is **Max rate**.

##### Tracking / Not Tracking
This check box controls the mount tracking. The mount tracking is enabled if it is checked.

##### Go Home
Checking this check box will slew the mount to its home position.

##### Parked / Unparked
Checking this checkbox will slew the mount to its park position.

##### Joystick Control
All available joysticks will be listed here. The mount can be controlled with the selected joystick/gamepad. The button operation of the joystick can be configured in the driver through *INDIGO Control Panel*.

### Object Tab
![](images_v2/telescope_object.png)

Typing in the **Search** field will update the object list as you type with the objects that match the pattern. Once the desired object is selected its coordinates will be loaded in telescope coordinate fields and clicking on **Goto** will point the telescope to the selected object.

Each object in the list has a tool tip with information about the object like coordinates, magnitude and type.

#### Manage Custom Objects
Custom objects can be added by clicking on the **[+]** button in the object tab. The following dialog will appear:

![](images_v2/add_object.png)

The data fields should be filled and the **Add object** button should be pushed to create the custom object.  The current telescope coordinates can be loaded in the **Right ascension** and **Declination** fields by pushing the target button.

Pushing the **[-]** button will remove the selected custom object. Please note that standard library objects can not be removed.

### Solver Tab
![](images_v2/telescope_solver.png)

In the solver tab the user should select the image source and exposure time to be used. Three operations can be performed:
- **Precise Goto** - the selected agent will slew the telescope to the input coordinates and then perform solve and center (available in version 1.2-0 and newer).
- **Solve & Center** - the selected agent will take an exposure, solve it, and using the solution as a reference will perform a precise goto to the target coordinates.
- **Solve & Sync** - the selected agent will take an exposure, solve it, and the solution will be sent to the telescope as a reference (sync the telescope with the solution). This will allow precise goto in the vicinity.

### Rotator Tab
![](images_v2/telescope_rotator.png)

In the **Rotator** drop-down menu, all available rotators will be listed. The selected rotator will be used.

The **Position** can be entered in the spinbox or by rotating the knob. The **[>]** button will move the rotator to the selected angle. The **[0]** button will sync the rotator to the selected angle.

If the **[+]** or **[-]** buttons are pressed, the rotator will move relative to the current position with the angle entered in the second spinbox.

**Start Derotation** will start the compensation for the field rotation introduced by the Alt-Azimuth mounts.

### Site (Geographic Location) Tab
![](images_v2/telescope_site.png)

In the **Source** drop-down menu, all available geo-location sources will be listed. Examples include the GPS device selected in the **GPS** tab or the selected **Mount Agent** coordinates, which are set in the **Set Location / Time** section.

If **Keep the mount time synchronized** is checked, whenever the time or geo coordinates of the source are changed, the mount will also be updated. This is somewhat dangerous as the GPS may lose its signal and the mount will lose alignment.

### GPS Tab
![](images_v2/telescope_gps.png)

In the **GPS** drop-down menu all available GPS devices will be listed. The selected GPS can be used as a time and geographic coordinates source in the **Site** tab. It also shows the position data from the selected device.

### Polar Align Tab
![](images_v2/telescope_polaralign.png)

Performing the telescope polar alignment procedure is described in detail in [INDIGO Astrometry / ASTAP Agent - Polar Alignment Guide](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/POLAR_ALIGNMENT.md).

## Plate Solving
Plate solving configuration and usage is described in [INDIGO Astrometry Agent - Plate Solving Guide](https://github.com/indigo-astronomy/indigo/blob/master/indigo_docs/PLATE_SOLVING.md) and in [Astrometry Agent README](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_astrometry/README.md)

![](images_v2/solver_main.png)

If a local file needs to be solved, **Image source** should be set to "Upload file". In this case, pushing the **Solve** button will display a file select dialog for choosing the file to be solved.

If the displayed image is solved a **Right-Click** on it will copy the coordinates under the mouse cursor in the **RA / Dec input** boxes and pushing **Goto** button will slew the telescope to these coordinates. **Control + Right-Click** will copy and slew the telescope in one action. This can be used to center a certain object in the frame. The telescope coordinates offset regarding to the solved image coordinates will be taken in to account, unless the solved image is an Uploaded file. In this case the solved coordinates will be used as most likely the telescope is pointing to another coordinates and the offset can not be calculated.

If you use the target button (next to the solution status indicator) to load the solved image center in to **RA / Dec input** fileds, the coordinates will NOT be adjusted to compensate for the telescope coordinates offset.

NOTE: If you want to get the same framing of an object as in a previous session, upload and solve a frame from this session and  **Right-Click** or **Control + Right-Click** on the image center and center the telescope to these coordinates as described above. Then if the rotation angle is different, rotate the camera until the same rotation angle is reached as the rotation ange of the reference frame.

## Accessing Advanced Device and Agent Settings

*Ain Imager* provides access to a subset of the standard properties of the devices and agents. The whole set of properties can be monitored and modified with the *INDIGO Control Panel*, free and open source application available for Linux and Windows.

 ![](images_v2/ICP.png)

 *Ain* provides a shortcut to it from **File -> INDIGO Control Panel**.

*INDIGO Control Panel* is essential for the initial service configuration like loading the necessary drivers and initial device configuration.

## Managing Service Configurations

Note: This feature is experimental!

*Ain Imager* can use *Configuration Agents* running on INDIGO services to manage the configuration of each service. It can save, load, delete and create new configurations for each INDIGO service. The configuration manager dialog can be accessed from **File -> Manage service configurations**.

![](images_v2/config_manager.png)

Available configuration agents will be listed in the top drop-down menu and available configurations will be listed in the **Configuration** combo box.

More information about configuration management can be found in [INDIGO Configuration Agent README](https://github.com/indigo-astronomy/indigo/blob/master/indigo_drivers/agent_config/README.md).
