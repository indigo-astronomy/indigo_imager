// Copyright (c) 2019 Rumen G.Bogdanovski
// All rights reserved.
//
// You can use this software under the terms of 'INDIGO Astronomy
// open-source license' (see LICENSE.md).
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHORS 'AS IS' AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


#ifndef CONF_H
#define CONF_H

#include <indigo/indigo_bus.h>
#include "blobpreview.h"

#if defined(INDIGO_WINDOWS)
#define PATH_LEN 4096
#else
#define PATH_LEN PATH_MAX
#endif

#define PROPERTY_AREA_MIN_WIDTH 500
#define CAMERA_FRAME_MIN_WIDTH 390
#define PREVIEW_WIDTH 550

#define CONFIG_FILENAME "indigo_control_panel.conf"

typedef struct {
	bool blobs_enabled;
	bool auto_connect;
	bool indigo_use_host_suffix;
	indigo_log_levels indigo_log_level;
	bool use_state_icons;
	bool use_system_locale;
	preview_stretch preview_stretch_level;
	char unused[1000];
} conf_t;

extern conf_t conf;
extern char config_path[PATH_LEN];

#endif // CONF_H
