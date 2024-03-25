#!/usr/bin/bash

cmake --build /home/sasetz/projects/cpp/psip_switch/build --target all && \
    sudo /home/sasetz/projects/cpp/psip_switch/build/psip_switch
