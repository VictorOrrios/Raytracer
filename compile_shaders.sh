#!/bin/bash

rm shaders/*.spv
glslc shaders/raytracer.comp -o shaders/raytracer.comp.spv

