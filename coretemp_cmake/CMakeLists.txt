# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(core_temp LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(systray
    main.cpp
    coretemp.cpp coretemp.h
)

target_link_libraries(systray PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
