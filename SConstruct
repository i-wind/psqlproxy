# pylint: disable=invalid-name,undefined-variable
"""
SQLProxy server build configuration
"""
env = Environment(
    CPPFLAGS='-m64 -pipe -O3 -g -Wall -W -std=c++11',
    LINKFLAGS='-m64 -g -Wl,-O3',
    CPPDEFINES='-D_DEBUG',
    CPPPATH=['/usr/local/include'],
    LIBPATH=['/usr/local/lib'],
    LIBS=Split('pthread boost_system boost_program_options boost_thread '
               'boost_filesystem boost_regex')
)

env.Program('sqlproxy', ['sqlproxy.cpp'])
