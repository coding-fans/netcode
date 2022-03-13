#!/bin/sh

# Author: fasion
# Created time: 2022-03-12 15:45:43
# Last Modified by: fasion
# Last Modified time: 2022-03-12 16:11:37

cd ideahub

export FLASK_APP=ideahub
export FLASK_ENV=development

tmux \
    set -g display-panes-time 10000 \; \
    set -g status-interval 1 \; \
    set -g status-right "#{?window_bigger,[#{window_offset_x}#,#{window_offset_y}] ,}\"#{=21:pane_title}\" %H:%M:%S" \; \
    new-session "flask run --host 0.0.0.0" \; \
    split-window -h -p 66 "zsh" \; \
