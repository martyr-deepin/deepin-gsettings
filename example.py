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

    def connect(self, name, func):
        return self.handle.connect(name, func)
    
    def list_keys(self):
        return self.handle.list_keys()
    
    def get_boolean(self, key):
        return self.handle.get_boolean(key)

    def set_boolean(self, key, value):
        return self.handle.set_boolean(key, value)

    def get_int(self, key):
        return self.handle.get_int(key)

    def set_int(self, key, value):
        return self.handle.set_int(key, value)

    def get_uint(self, key):
        return self.handle.get_uint(key)

    def get_strv(self, key):
        return self.handle.get_strv(key)

    def set_strv(self, key, value):
        return self.handle.set_strv(key, value)

class DownladThread(td.Thread):
    def __init__(self, schema_id):
        td.Thread.__init__(self)
        self.setDaemon(True)
        self.deepin_gsettings = DeepinGSettings(schema_id)

    def run(self):
        pass

def m_changed():
    print "DEBUG changed xxxxxxxxxxxxxxxxxxxxxxxx"

deepin_gsettings_instance1 = DeepinGSettings("org.gnome.settings-daemon.plugins.power")
deepin_gsettings_instance1.connect("changed", m_changed)
deepin_gsettings_instance2 = DeepinGSettings("org.gnome.libgnomekbd.keyboard")
deepin_gsettings_instance2.connect("changed", m_changed)
print "list_keys ", deepin_gsettings_instance1.list_keys()
print "get_boolean active ", deepin_gsettings_instance1.get_boolean("active")
print "set_boolean idle-dim-battery ", deepin_gsettings_instance1.set_boolean("idle-dim-battery", True)
print "get_int idle-brightness ", deepin_gsettings_instance1.get_int("idle-brightness")
print "set_int idle-brightness ", deepin_gsettings_instance1.set_int("idle-brightness", 31)
print "get_strv layouts ", deepin_gsettings_instance2.get_strv("options")
print "set_strv options ", deepin_gsettings_instance2.set_strv("options", ['xxx', 'ooo'])
