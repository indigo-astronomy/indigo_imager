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

#define set_alert(widget) (widget->setStyleSheet("*:enabled { background-color: #312222;} *:!enabled { background-color: #292222;}"))
#define set_idle(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} *:!enabled {background-color: #272727;}"))
#define set_busy(widget) (widget->setStyleSheet("*:enabled {background-color: #313120;} *:!enabled {background-color: #292920;}"))
#define set_ok(widget) (widget->setStyleSheet("*:enabled {background-color: #272727;} QSpinBox:!enabled {background-color: #202020;}"))
#define set_ok2(widget) (widget->setStyleSheet("background-color: #273527;"))

#endif /* _WIDGET_STATE_H */
