// Copyright (c) 2020 Rumen G.Bogdanovski
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

#ifndef _WIDGET_STATE_H
#define _WIDGET_STATE_H

#define set_alert(widget) (widget->setStyleSheet("*:enabled { background-color: #342424;} *:!enabled { background-color: #322424;}"))
#define set_idle(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} *:!enabled {background-color: #272727;}"))
#define set_busy(widget) (widget->setStyleSheet("*:enabled {background-color: #343422;} *:!enabled {background-color: #323222;}"))
#define set_ok(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} QSpinBox:!enabled {background-color: #222222;}"))
#define set_ok2(widget) (widget->setStyleSheet("background-color: #273727;"))


#define SAVE_BOTH_INDICATOR            "<font color='#3b9640'><b>●</b></font>"
#define SAVE_REMOTE_INDICATOR          "<font color='#3b96ff'><b>●</b></font>"
#define DOWNLOAD_INDICATOR             "<font color='#3b9640'><b>●</b></font>"

#ifdef INDIGO_WINDOWS
	#define SAVE_LOCAL_INDICATOR           "<font color='#3b9640'><b>◎</b></font>"
	#define PREVIEW_REMOTE_INDICATOR       "<font color='#3b96ff'><b>◎</b></font>"
	#define DOWNLOAD_REMOVE_INDICATOR      "<font color='#3b9640'><b>◎</b></font>"
#else
	#define SAVE_LOCAL_INDICATOR           "<font color='#3b9640'><b>◉</b></font>"
	#define PREVIEW_REMOTE_INDICATOR       "<font color='#3b96ff'><b>◉</b></font>"
	#define DOWNLOAD_REMOVE_INDICATOR      "<font color='#3b9640'><b>◉</b></font>"
#endif

#endif /* _WIDGET_STATE_H */
