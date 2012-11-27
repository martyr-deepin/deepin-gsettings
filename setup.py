#! /usr/bin/env python

from setuptools import setup, Extension
import os
import commands

def pkg_config_cflags(pkgs):
    '''List all include paths that output by `pkg-config --cflags pkgs`'''
    return map(lambda path: path[2::], commands.getoutput('pkg-config --cflags-only-I %s' % (' '.join(pkgs))).split())

deepin_gsettings_mod = Extension('deepin-gsettings', 
                include_dirs = pkg_config_cflags(['gio-2.0']),
                libraries = ['gio-2.0'],
                sources = ['deepin_gsettings_python.c'])

setup(name='deepin-gsettings',
      version='0.1',
      ext_modules = [deepin_gsettings_mod],
      description='Deepin Gsettings Python binding.',
      long_description ="""Deepin Gsettings Python binding.""",
      author='Linux Deepin Team',
      author_email='zhaixiang@linuxdeepin.com',
      license='GPL-3',
      url="https://github.com/linuxdeepin/deepin-gsettings",
      download_url="git@github.com:linuxdeepin/deepin-gsettings.git",
      platforms = ['Linux'],
      )

