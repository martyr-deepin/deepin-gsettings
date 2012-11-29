#! /usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2012 Deepin, Inc.
#               2012 Zhai Xiang
# 
# Author:     Zhai Xiang <zhaixiang@linuxdeepin.com>
# Maintainer: Zhai Xiang <zhaixiang@linuxdeepin.com>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import deepin_gsettings
import threading as td

class DeepinGSettings:
    def __init__(self, schema_id):
        '''
        DeepinGSettings construction
        @para schema_id
        '''
        self.handle = deepin_gsettings.new(schema_id)

    def __del__(self):
        self.handle.delete()
        self.handle = None

    def get_boolean(self, key):
        return self.handle.get_boolean(key)

class DownladThread(td.Thread):
    def __init__(self, schema_id):
        td.Thread.__init__(self)
        self.setDaemon(True)
        self.deepin_gsettings = DeepinGSettings(schema_id)

    def run(self):
        pass

deepin_gsettings_instance1 = DeepinGSettings("org.gnome.settings-daemon.plugins.power")
print deepin_gsettings_instance1.get_boolean("active")
