#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0+
# Copyright (c) 2022-2023 Texas Instruments Incorporated - https://www.ti.com/
# Written by Neha Malcom Francis <n-francis@ti.com>
#
# Entry-type module for generating schema validated TI board
# configuration binary
#

import os
import struct
import yaml
import yamllint
import sys

from jsonschema import validate

from yamllint import config

BOARDCFG = 0xB
BOARDCFG_SEC = 0xD
BOARDCFG_PM = 0xE
BOARDCFG_RM = 0xC

class cfgentry:
    def __init__(self, cfgtype, data):
        self.cfgtype = cfgtype
        self.data = data

class Entry_ti_board_config:
    def __init__(self, schema):
        self._config = None
        self._schema = None
        self._fmt = '<HHHBB'
        self._index = 0
        self._sw_rev = 1
        self._devgrp = 0
        self.cfgentries = []
        self.header = struct.pack('<BB', 4, 1)
        self._binary_offset = len(self.header)
        self._schema_file = schema

    def _convert_to_byte_chunk(self, val, data_type):
        """Convert value into byte array

        Args:
            val: value to convert into byte array
            data_type: data type used in schema, supported data types are u8,
                u16 and u32

        Returns:
            array of bytes representing value
        """
        size = 0
        if (data_type == '#/definitions/u8'):
            size = 1
        elif (data_type == '#/definitions/u16'):
            size = 2
        else:
            size = 4
        if type(val) == int:
            br = val.to_bytes(size, byteorder='little')
        return br

    def _compile_yaml(self, schema_yaml, file_yaml):
        """Convert YAML file into byte array based on YAML schema

        Args:
            schema_yaml: file containing YAML schema
            file_yaml: file containing config to compile

        Returns:
            array of bytes repesenting YAML file against YAML schema
        """
        br = bytearray()
        for key, node in file_yaml.items():
            node_schema = schema_yaml['properties'][key]
            node_type = node_schema.get('type')
            if not 'type' in node_schema:
                br += self._convert_to_byte_chunk(node,
                                                node_schema.get('$ref'))
            elif node_type == 'object':
                br += self._compile_yaml(node_schema, node)
            elif node_type == 'array':
                for item in node:
                    if not isinstance(item, dict):
                        br += self._convert_to_byte_chunk(
                            item, schema_yaml['properties'][key]['items']['$ref'])
                    else:
                        br += self._compile_yaml(node_schema.get('items'), item)
        return br

    def _generate_binaries(self):
        """Generate config binary artifacts from the loaded YAML configuration file

        Returns:
            byte array containing config binary artifacts
            or None if generation fails
        """
        cfg_binary = bytearray()
        for key, node in self.file_yaml.items():
            node_schema = self.schema_yaml['properties'][key]
            br = self._compile_yaml(node_schema, node)
            cfg_binary += br
        return cfg_binary

    def _add_boardcfg(self, bcfgtype, bcfgdata):
        """Add board config to combined board config binary

        Args:
            bcfgtype (int): board config type
            bcfgdata (byte array): board config data
        """
        size = len(bcfgdata)

        desc = struct.pack(self._fmt, bcfgtype,
                            self._binary_offset, size, self._devgrp, 0)
        self._binary_offset += size
        self._index += 1
        return desc

    def add_data(self, configfile):
        self._config_file = configfile
        with open(self._config_file, 'r') as f:
            self.file_yaml = yaml.safe_load(f)
        with open(self._schema_file, 'r') as sch:
            self.schema_yaml = yaml.safe_load(sch)

        if self.file_yaml.get('board-cfg') != None:
            cfgtype = BOARDCFG
        if self.file_yaml.get('sec-cfg') != None:
            cfgtype = BOARDCFG_SEC
        if self.file_yaml.get('pm-cfg') != None:
            cfgtype = BOARDCFG_PM
        if self.file_yaml.get('rm-cfg') != None:
            cfgtype = BOARDCFG_RM

        yaml_config = config.YamlLintConfig("extends: default")
        for p in yamllint.linter.run(open(self._config_file, "r"), yaml_config):
            self.Raise(f"Yamllint error: {p.line}: {p.rule}")
        try:
            validate(self.file_yaml, self.schema_yaml)
        except Exception as e:
            self.Raise(f"Schema validation error: {e}")

        data = self._generate_binaries()
        entry = cfgentry(cfgtype, data)
        self.cfgentries.append(entry)
        return data

    def save(self, filename):
        with open(filename, "wb") as binary_file:
            binary_file.write(self.header)
            for i in self.cfgentries:
                obj._binary_offset += 8
            for i in self.cfgentries:
                binary_file.write(self._add_boardcfg(i.cfgtype, i.data))
            for i in self.cfgentries:
                binary_file.write(i.data)
            binary_file.close()

outfile = sys.argv[1]
schema = sys.argv[2]

obj = Entry_ti_board_config(schema)

for i in sys.argv[3:]:
    obj.add_data(i)

obj.save(outfile)
