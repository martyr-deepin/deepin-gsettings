env = Environment(CCFLAGS='-g')
env.ParseConfig('pkg-config --cflags --libs gtk+-2.0 gio-2.0')

env.Program('hello_gsettings', ['hello_gsettings.c'])
