#!/usr/bin/env python
# Creates C #define statements for IPFIX Information Elements specified by IANA
# Copyright (C) 2014 Oliver Gasser


import argparse
import csv
import os
import os.path
import sys
import tempfile
import urllib.request

IANA_IE_URL = "http://www.iana.org/assignments/ipfix/ipfix-information-elements.csv"
IANA_DATA_TYPES_URL = "http://www.iana.org/assignments/ipfix/ipfix-information-element-data-types.csv"

class IanaDialect(csv.Dialect):
    # ElementID,Name,Data Type,Data Type Semantics,Status,Description,Units,Range,References,Requester,Revision,Date^M
    strict = True
    skipinitialspace = True
    delimiter = ','
    quotechar = '"'



def parse_iana_csv_ie(fh, out, struct_out):
    # ElementID,Name,Data Type,Data Type Semantics,Status,Description,Units,Range,References,Requester,Revision,Date^M
    reader = csv.reader(fh)
    ids = ""
    types = ""
    lengths = ""
    struct = "struct ipfix_identifier ipfixids_iana[] = {\n/* IANA registry */\n"
    for row in reader:
        if row[4] == 'current':
            ids, types, lengths, struct = append_output(row, ids, types, lengths, struct)

    struct += "};\n\n"

    out.write(ids)
    out.write("\n")
    out.write(lengths)
    out.write("\n")
    out.write(types)
    out.write("\n")
    struct_out.write(struct)


def parse_iana_csv_data_types(fh, out):
    # Value,Description,Reference
    reader = csv.reader(fh)
    for row in reader:
        if row[2] != "" and row[0] != "Value":
            out.write("#define IPFIX_TYPE_{0:37} {1}\n".format(row[1], row[0]))

    out.write("\n")

    length_macros = ""

    for t in ("octetArray", "unsigned8", "unsigned16", "unsigned32", "unsigned64", "signed8", "signed16", "signed32", "signed64", "float32", "float64", "boolean", "macAddress", "string", "dateTimeSeconds", "dateTimeMilliseconds", "dateTimeMicroseconds", "dateTimeNanoseconds", "ipv4Address", "ipv6Address", "basicList", "subTemplateList", "subTemplateMultiList"):
        length_macros += "#define IPFIX_BYTES_{0:25} {1}\n".format(t, type_to_length(t))

    out.write(length_macros)
    out.write("\n")


def append_output(row, ids, types, lengths, struct):
    elementId = row[0]
    name = row[1]
    dataType = row[2]

    ids += "#define IPFIX_TYPEID_{0:40} {1}\n".format(name, elementId)
    types += "#define IPFIX_DATA_TYPE_{0:40} IPFIX_TYPE_{1}\n".format(name, dataType)
    lengths += "#define IPFIX_LENGTH_{0:40} IPFIX_BYTES_{1}\n".format(name, dataType)
    struct += '  {{ IPFIX_TYPEID_{0:40}, IPFIX_BYTES_{1:20}, {2:5}, {3:40}, IPFIX_DATA_TYPE_{4:40} }},\n'.format(name, dataType, 0, '"' + name + '"', name)

    return ids, types, lengths, struct


def type_to_length(data_type):
    if data_type == "unsigned8" or data_type == "signed8" or data_type == "boolean":
        return 1
    if data_type == "unsigned16" or data_type == "signed16":
        return 2
    if data_type == "unsigned32" or data_type == "signed32" or data_type == "float32" or data_type == "dateTimeSeconds" or data_type == "ipv4Address":
        return 4
    if data_type == "macAddress":
        return 6
    if data_type == "unsigned64" or data_type == "signed64" or data_type == "float64" or data_type == "dateTimeMilliseconds" or data_type == "dateTimeMicroseconds" or data_type == "dateTimeNanoseconds":
        return 8
    if data_type == "ipv6Address":
        return 16
    if data_type == "octetArray" or data_type == "string" or data_type == "basicList" or data_type == "subTemplateList" or data_type == "subTemplateMultiList":
        return 65535

    raise Exception("Could not convert data type " + data_type)


def generate_ipfix_output(ie_fh, types_fh, out, struct_out):
    # Header
    out.write('#ifndef IPFIX_IANA_H\n#define IPFIX_IANA_H\n\n#include "ipfix_names.h"\n\n')
    struct_out.write('#include "ipfix_iana.h"\n\n#ifdef __cplusplus\nextern "C" {\n#endif\n\n')

    parse_iana_csv_data_types(types_fh, out)
    parse_iana_csv_ie(ie_fh, out, struct_out)

    # Footer
    struct_out.write('#ifdef __cplusplus\n}\n#endif\n')
    out.write('\n#endif\n')

    # Close outputs
    out.close()
    struct_out.close()


def download_csvs():
    for url in (IANA_IE_URL, IANA_DATA_TYPES_URL):
        response = urllib.request.urlopen(url)
        data = response.read().decode('utf-8')
        fh = tempfile.TemporaryFile(mode='w+t')
        fh.write(data)
        fh.seek(0)
        yield fh


def main():
    parser = argparse.ArgumentParser(description='Create C output for IPFIX Information Elements to be used in Vermont.')
    parser.add_argument('-i', '--ie-file', metavar='IE_CSV', type=argparse.FileType('r'), help="CSV file containing IPFIX Information Elements specified by IANA.")
    parser.add_argument('-t', '--type-file', metavar='DATA_TYPES_CSV', type=argparse.FileType('r'), help="CSV file containing IPFIX Data Types specified by IANA.")
    parser.add_argument('-u', '--update', action='store_true', help="Update specified IPFIX Information Elements from IANA website.")
    parser.add_argument('-d', '--directory', metavar='DIR',  help="Write to files in DIR instead of STDOUT.")

    args = parser.parse_args()

    # Check for correct arguments
    if (args.ie_file and args.type_file and not args.update) or (not args.ie_file and not args.type_file and args.update):
        pass
    else:
        parser.print_usage()
        sys.exit(1)

    # Update CSVs from IANA website
    if args.update:
        args.ie_file, args.type_file = download_csvs()

    # Default output is standard out
    out = sys.stdout
    struct_out = sys.stdout

    # Create new files for output
    if args.directory and os.path.isdir(args.directory):
        out = open(os.path.join(args.directory, 'ipfix_iana.h'), 'w')
        struct_out = open(os.path.join(args.directory, 'ipfix_iana.c'), 'w')

    generate_ipfix_output(args.ie_file, args.type_file, out, struct_out)


if __name__ == '__main__':
    main()


# vim: ts=4 sw=4 et ai
