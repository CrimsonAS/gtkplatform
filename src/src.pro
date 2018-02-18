TEMPLATE = subdirs
SUBDIRS += \
    gtkextras \
    platform_plugin

platform_plugin.subdir = platform-plugin
platform_plugin.depends = gtkextras
