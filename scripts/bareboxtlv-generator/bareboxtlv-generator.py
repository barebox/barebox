#!/usr/bin/env python3

import argparse
import struct

import yaml
from crcmod.predefined import mkPredefinedCrcFun

_crc32_mpeg = mkPredefinedCrcFun("crc-32-mpeg")


class MaxSizeReachedException(Exception):
    pass


class FactoryDataset:
    """
    Generates TLV-encoded datasets that can be used with Barebox's
    TLV support.

    The generated binary structure looks like this:
    #############################################################
    # struct tlv {                                              #
    #     be16 tag; /* 2 bytes */                               #
    #     be16 len; /* 2 bytes */                               #
    #     u8 payload[];                                         #
    # };                                                        #
    #                                                           #
    # struct binfile {                                          #
    #     be32 magic_version;                                   #
    #     be32 length_tlv; /* 4 bytes */                        #
    #     be32 length_sig; /* 4 bytes */                        #
    #     struct tlv tlvs[];                                    #
    #     be32 crc32;                                           #
    # };                                                        #
    #############################################################

    Limitations:
    * Signing is currently not supported and length_sig is always 0x0.
    """

    def __init__(self, schema):
        """
        Create a new Factory Dataset Object.

        A ``schema`` is the python-world representation of the Barebox
        ``struct tlv_mapping`` lists.
        It describes the tags that can be used and define which
        formatter must be used when (de-) serializing data.

        Example schema:
        | magic: 0x0fe01fca
        | max_size: 0x100
        |
        | tags:
        |   factory-timestamp:
        |     tag: 0x0003
        |     format: "decimal"
        |     length: 8
        |   (...)

        With:
        * magic: The magic header for the TLV-encoded data.
        * max_size: Maximum length of the TLV-encoded data in bytes.
                    If set the generated binary is tested to be less or equal this size after generation.
        * tags: dict of tags that can be used for TLV-encoding.
          * <key>: The human-readable name of this field.
          * tag: (Mandatory) Tag-ID for use during TLV-encoding
          * format: (Mandatory) Specifies the formatter to use
          * length: (Mandatory for some formats) length of the field

        Some remarks:
        * Additional keys in a tag will be ignored. This can be used to add examples or annotations
          to an entry.
        * Tag-IDs can be used in multiple entries inside a schema with different human-readable names.
          This only works reasonable if the Barebox parser knows how to handle it.
          This will work well during encoding. But: It will need the same special knowledge for decoding.
          Otherwise, only the last occurrence will be kept.
        * The EEPROM will be written in the order of entries in the input data.

        Supported Barebox TLV data formats:
        * string:
          - Arbitrary length string (up to 2^16-1 bytes)
          - UTF-8
          - Schema example:
            |  device-hardware-release:
            |    tag: 0x0002
            |    format: "string"
        * decimal:
          - unsigned int
          - length is required and must be in [1, 2, 4, 8]
          - Schema example:
            |  modification:
            |    tag: 0x0005
            |    format: decimal
            |    length: 1
        * mac-sequence:
          - Describes a consecutive number of MAC addresses
          - Contains a starting address and a count
          - Input data must be an iterable in the following format: (first_mac: int, count: int)
          - MAC-addresses are represented as python ints
          - Schema example:
            |  ethernet-address:
            |    tag: 0x0012
            |    format: "mac-sequence"
        * mac-list:
          - A list of MAC addresses
          - Input data must be an iterable of MAC addresses: (first_mac: int, second_mac: int, ...)
          - MAC-addresses are represented as python ints
          - Schema example:
            |  ethernet-address:
            |    tag: 0x0012
            |    format: "mac-list"
        * linear-calibration
          - Linear calibration data for analog channels
          - Input data must be an iterable of floats: (c1: float, c2: float, ...)
          - Values are stored as a 4-byte float
          - Length-field is mandatory.
          - Schema example:
            |  usb-host1-curr:
            |    tag: 0x8001
            |    format: linear-calibration
            |    length: 2
        """
        self.schema = schema

    def encode(self, data):
        """
        Generate an EEPROM image for the given data.

        Data must be a dict using fields described in the schema.
        """
        # build payload of TLVs
        tlvs = b""

        for name, value in data.items():
            if name not in self.schema["tags"]:
                raise KeyError(f"No Schema defined for data with name '{name}'")

            tag = self.schema["tags"][name]
            tag_format = tag["format"]

            if tag_format == "string":
                bin = value.encode("UTF-8")
                fmt = f"{len(bin)}s"
                if len(bin) > 2**16 - 1:
                    raise ValueError(f"String {name} is too long!")

            elif tag_format == "decimal":
                fmtl = tag["length"]
                if fmtl == 1:
                    fmt = "B"
                elif fmtl == 2:
                    fmt = "H"
                elif fmtl == 4:
                    fmt = "I"
                elif fmtl == 8:
                    fmt = "Q"
                else:
                    raise ValueError(f"Decimal {name} has invalid len {fmtl}. Must be in [1, 2, 4, 8]!")
                bin = abs(int(value))

            elif tag_format == "mac-sequence":
                if len(value) != 2:
                    raise ValueError(f"mac-sequence {name} must be in format (base_mac: int, count: int)")
                fmt = "7s"
                mac = struct.pack(">Q", value[0])[2:]
                bin = struct.pack(">B6s", value[1], mac)

            elif tag_format == "mac-list":
                bin = b""
                for mac in value:
                    bin += struct.pack(">Q", mac)[2:]
                fmt = f"{len(value) * 6}s"

            elif tag_format == "calibration":
                bin = b""
                if len(value) != tag["length"]:
                    raise ValueError(f'linear-calibration {name} must have {tag["length"]} elements.')
                for coeff in value:
                    bin += struct.pack(">f", coeff)
                fmt = f"{len(value) * 4}s"

            else:
                raise Exception(f"{name}: Unknown format {tag_format}")

            tlvs += struct.pack(f">HH{fmt}", tag["tag"], struct.calcsize(fmt), bin)

        # Convert the framing data.
        len_sig = 0x0  # Not implemented.
        header = struct.pack(">3I", self.schema["magic"], len(tlvs), len_sig)
        crc_raw = _crc32_mpeg(header + tlvs)
        crc = struct.pack(">I", crc_raw)
        bin = header + tlvs + crc

        # Check length
        if "max_size" in self.schema and len(bin) > self.schema["max_size"]:
            raise MaxSizeReachedException(
                f'Generated binary too large. Is {len(bin)} bytes but max_size is {self.schema["max_size"]}.'
            )
        return bin

    def decode(self, bin):
        """
        Decode a TLV-encoded binary image.

        Returns a dict with entries decoded from the binary.
        """
        # check minimal length
        values = {}

        if len(bin) < 16:
            raise ValueError("Supplied binary is too small to be TLV-encoded data.")

        bin_magic, bin_tlv_len, bin_sig_len = struct.unpack(">3I", bin[:12])
        # check magic
        if bin_magic != self.schema["magic"]:
            raise ValueError(f'Magic missmatch. Is {hex(bin_magic)} but expected {hex(self.schema["magic"])}')

        # check signature length
        if bin_sig_len != 0:
            raise ValueError("Signature check is not supported!")

        # check crc
        crc_offset = 12 + bin_tlv_len + bin_sig_len
        if crc_offset > len(bin) - 4:
            raise ValueError("crc location calculated behind binary.")
        bin_crc = struct.unpack(">I", bin[crc_offset : crc_offset + 4])[0]  # noqa E203
        calc_crc = _crc32_mpeg(bin[:crc_offset])
        if bin_crc != calc_crc:
            raise ValueError(f"CRC missmatch. Is {hex(bin_crc)} but expected {hex(calc_crc)}.")

        ptr = 12
        while ptr < crc_offset:
            tag_id, tag_len = struct.unpack_from(">HH", bin, ptr)
            data_ptr = ptr + 4
            ptr += tag_len + 4

            name, tag_schema = self._get_tag_by_id(tag_id)
            if not name:
                print(f"Skipping unknown tag-id {hex(tag_id)}.")
                continue

            if tag_schema["format"] == "decimal":
                if tag_len == 1:
                    fmt = ">B"
                elif tag_len == 2:
                    fmt = ">H"
                elif tag_len == 4:
                    fmt = ">I"
                elif tag_len == 8:
                    fmt = ">Q"
                else:
                    raise ValueError(f"Decimal {name} has invalid len {tag_len}. Must be in [1, 2, 4, 8]!")
                value = struct.unpack_from(fmt, bin, data_ptr)[0]
            elif tag_schema["format"] == "string":
                value = bin[data_ptr : data_ptr + tag_len].decode("UTF-8")  # noqa E203
            elif tag_schema["format"] == "mac-sequence":
                if tag_len != 7:
                    raise ValueError(f"Tag {name} has wrong length {hex(tag_len)} but expected 0x7.")
                count, base_mac = struct.unpack_from(">B6s", bin, data_ptr)
                base_mac = struct.unpack(">Q", b"\x00\x00" + base_mac)[0]
                value = [base_mac, count]
            elif tag_schema["format"] == "mac-list":
                if tag_len % 6 != 0:
                    raise ValueError(f"Tag {name} has wrong length {hex(tag_id)}. Must be multiple of 0x6.")
                value = []
                for i in range(int(tag_len / 6)):
                    mac = struct.unpack_from(">6s", bin, data_ptr + int(i * 6))[0]
                    mac = struct.unpack(">Q", b"\x00\x00" + mac)[0]
                    value.append(mac)
            elif tag_schema["format"] == "calibration":
                if tag_len % 4 != 0:
                    raise ValueError(f"Tag {name} has wrong length {hex(tag_id)}. Must be multiple of 0x4.")
                value = []
                for i in range(int(tag_len / 4)):
                    coeff = struct.unpack_from(">f", bin, data_ptr + int(i * 4))[0]
                    value.append(coeff)
            else:
                raise Exception(f'{name}: Unknown format {tag_schema["format"]}')
            values[name] = value

        return values

    def _get_tag_by_id(self, tag_id):
        for name, tag_schema in self.schema["tags"].items():
            if tag_schema["tag"] == tag_id:
                return name, tag_schema
        return None, None


def _main():
    parser = argparse.ArgumentParser(description="Generate a TLV dataset for the Barebox TLV parser")
    parser.add_argument("schema", help="YAML file describing the data.")
    parser.add_argument("--input-data", help="YAML file containing data to write to the binary.")
    parser.add_argument("--output-data", help="YAML file where the contents of the binary will be written to.")
    parser.add_argument("binary", help="Path to where export data to be copied into DUT's EEPROM.")
    args = parser.parse_args()

    # load schema
    with open(args.schema) as schema_fh:
        schema = yaml.load(schema_fh, Loader=yaml.SafeLoader)
    eeprom = FactoryDataset(schema)

    if args.input_data:
        with open(args.input_data) as d_fh:
            data = yaml.load(d_fh, Loader=yaml.SafeLoader)
        bin = eeprom.encode(data)
        with open(args.binary, "wb+") as out_fh:
            out_fh.write(bin)

    if args.output_data:
        with open(args.binary, "rb") as in_fh:
            bin = in_fh.read()
        data = eeprom.decode(bin)
        with open(args.output_data, "w+") as out_fh:
            yaml.dump(data, out_fh)


if __name__ == "__main__":
    _main()
