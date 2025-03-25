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

#define IMAGE_AREA_MIN_WIDTH 500
#define TOOLBAR_MIN_WIDTH 420

#define CONFIG_FILENAME "indigo_imager.conf"
#define AIN_GUIDER_LOG_NAME_FORMAT "ain_guiding_%s.log"
#define AIN_INDIGO_LOG_NAME_FORMAT "ain_indigo_%s.log"
#define DEFAULT_OBJECT_NAME "unknown"

#define AIN_USERS_GUIDE_URL "https://github.com/indigo-astronomy/indigo_imager/blob/master/ain_users_guide/ain_v2_users_guide.md"

#define AIN_SEQUENCE_NAME "AinSequence"

typedef enum {
	STRETCH_NONE = 0,
	STRETCH_SLIGHT = 1,
	STRETCH_MODERATE = 2,
	STRETCH_NORMAL = 3,
	STRETCH_HARD = 4,
} preview_stretch;

typedef enum {
	CB_AUTO = 0,
	CB_NONE
} color_balance;

typedef enum {
	SHOW_FWHM = 0,  // Not used any more
	SHOW_HFD = 1,
	SHOW_CONTRAST = 2,
	SHOW_BAHTINOV = 3,
} focuser_display_data;

typedef enum {
	SHOW_RA_DEC_DRIFT = 0,
	SHOW_RA_DEC_S_DRIFT = 1,
	SHOW_RA_DEC_PULSE = 2,
	SHOW_X_Y_DRIFT = 3,
} guider_display_data;

typedef enum {
	AIN_ALERT_STATE = INDIGO_ALERT_STATE,
	AIN_WARNING_STATE = INDIGO_BUSY_STATE,
	AIN_OK_STATE = 100
} object_alt_state;

typedef enum {
	AIN_NO_SOUND = 0,
	AIN_ALERT_SOUND,
	AIN_WARNING_SOUND,
	AIN_OK_SOUND
} ain_sounds;

typedef struct {
	bool blobs_enabled;
	bool auto_connect;
	bool indigo_use_host_suffix;
	indigo_log_levels indigo_log_level;
	bool use_state_icons;
	bool use_system_locale;
	bool antialiasing_enabled;
	int focus_mode;
	preview_stretch preview_stretch_level;
	int guider_save_bandwidth;
	int guider_subframe;
	int focuser_subframe;
	preview_stretch guider_stretch_level;
	bool guider_antialiasing_enabled;
	focuser_display_data focuser_display;    // Not used any more
	guider_display_data guider_display;
	bool guider_save_log;
	bool indigo_save_log;
	char solver_image_source1[INDIGO_NAME_SIZE];
	char solver_image_source2[INDIGO_NAME_SIZE];
	bool save_noname_images;
	char data_dir_prefix[PATH_LEN];
	color_balance preview_color_balance;
	color_balance guider_color_balance;
	int window_width;
	int window_height;
	bool restore_window_size;
	bool imager_show_reference;
	char solver_image_source3[INDIGO_NAME_SIZE];
	char sound_notification_level;
	bool save_images_on_server;
	bool keep_images_on_server;
	bool statistics_enabled;
	uint32_t preview_bayer_pattern; /* BAYER_PAT_XXXX from image_preview_lut.h */
	bool require_confirmation;
	bool compact_window_layout;
	char unused[100];
} conf_t;

extern conf_t conf;
extern char config_path[PATH_LEN];

//#define USE_LCD

#endif // CONF_H
