env = Environment(CCFLAGS='-g')
env.ParseConfig('pkg-config --cflags --libs gio-2.0')

env.Program('hello_gsettings', ['hello_gsettings.c'])
