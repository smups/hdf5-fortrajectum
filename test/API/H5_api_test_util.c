/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "H5_api_test.h"
#include "H5_api_test_util.h"

/*
 * The maximum allowable size of a generated datatype.
 *
 * NOTE: HDF5 currently has limits on the maximum size of
 * a datatype of an object, as this information is stored
 * in the object header. In order to provide maximum
 * compatibility between the native VOL connector and others
 * for this test suite, we limit the size of a datatype here.
 * This value should be adjusted as future HDF5 development
 * allows.
 */
#define GENERATED_DATATYPE_MAX_SIZE 65536

/*
 * The maximum size of a datatype for compact objects that
 * must fit within the size of a native HDF5 object header message.
 * This is typically used for attributes and compact datasets.
 */
#define COMPACT_DATATYPE_MAX_SIZE 1024

/* The maximum level of recursion that the generate_random_datatype()
 * function should go down to, before being forced to choose a base type.
 */
#define TYPE_GEN_RECURSION_MAX_DEPTH 3

/* The number of predefined standard integer types in HDF5
 */
#define NUM_PREDEFINED_INT_TYPES 16

/* The number of predefined floating point types in HDF5
 */
#define NUM_PREDEFINED_FLOAT_TYPES 4

/* The maximum number of members allowed in an HDF5 compound type, as
 * generated by the generate_random_datatype() function, for ease of
 * development.
 */
#define COMPOUND_TYPE_MAX_MEMBERS 4

/* The maximum number and size of the dimensions of an HDF5 array
 * datatype, as generated by the generate_random_datatype() function.
 */
#define ARRAY_TYPE_MAX_DIMS 4

/* The maximum number of members and the maximum size of those
 * members' names for an HDF5 enum type, as generated by the
 * generate_random_datatype() function.
 */
#define ENUM_TYPE_MAX_MEMBER_NAME_LENGTH 256
#define ENUM_TYPE_MAX_MEMBERS            16

/* The maximum size of an HDF5 string datatype, as created by the
 * generate_random_datatype() function.
 */
#define STRING_TYPE_MAX_SIZE 1024

/*
 * The maximum dimensionality and dimension size of a dataspace
 * generated for an attribute or compact dataset.
 */
#define COMPACT_SPACE_MAX_DIM_SIZE 4
#define COMPACT_SPACE_MAX_DIMS     3

typedef hid_t (*generate_datatype_func)(H5T_class_t parent_class, bool is_compact);

static hid_t generate_random_datatype_integer(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_float(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_string(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_compound(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_reference(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_enum(H5T_class_t parent_class, bool is_compact);
static hid_t generate_random_datatype_array(H5T_class_t parent_class, bool is_compact);

/*
 * Helper function to generate a random HDF5 datatype in order to thoroughly
 * test support for datatypes. The parent_class parameter is to support
 * recursive generation of datatypes. In most cases, this function should be
 * called with H5T_NO_CLASS for the parent_class parameter.
 */
hid_t
generate_random_datatype(H5T_class_t parent_class, bool is_compact)
{
    generate_datatype_func gen_func;
    static int             depth = 0;
    size_t                 type_size;
    hid_t                  datatype  = H5I_INVALID_HID;
    hid_t                  ret_value = H5I_INVALID_HID;

    depth++;

roll_datatype:
    switch (rand() % H5T_NCLASSES) {
        case H5T_INTEGER:
            gen_func = generate_random_datatype_integer;
            break;
        case H5T_FLOAT:
            gen_func = generate_random_datatype_float;
            break;
        case H5T_TIME:
            /* Time datatype is unsupported, try again */
            goto roll_datatype;
            break;
        case H5T_STRING:
            gen_func = generate_random_datatype_string;
            break;
        case H5T_BITFIELD:
            /* Bitfield datatype is unsupported, try again */
            goto roll_datatype;
            break;
        case H5T_OPAQUE:
            /* Opaque datatype is unsupported, try again */
            goto roll_datatype;
            break;
        case H5T_COMPOUND:
            /* Currently only allows arrays of integer, float or string. Pick another type if we
             * are creating an array of something other than these. Also don't allow recursion
             * to go too deep. Pick another type that doesn't recursively call this function. */
            if ((H5T_ARRAY == parent_class) || ((depth + 1) > TYPE_GEN_RECURSION_MAX_DEPTH))
                goto roll_datatype;

            gen_func = generate_random_datatype_compound;
            break;
        case H5T_REFERENCE:
            /* Temporarily disable generation of reference datatypes */
            goto roll_datatype;

            /* Currently only allows arrays of integer, float or string. Pick another type if we
             * are creating an array of something other than these. */
            if (H5T_ARRAY == parent_class)
                goto roll_datatype;

            gen_func = generate_random_datatype_reference;
            break;
        case H5T_ENUM:
            /* Currently doesn't currently support ARRAY of ENUM, so try another type
             * if this happens. */
            if (H5T_ARRAY == parent_class)
                goto roll_datatype;

            gen_func = generate_random_datatype_enum;
            break;
        case H5T_VLEN:
            /* Variable-length datatypes are unsupported, try again */
            goto roll_datatype;
            break;
        case H5T_ARRAY:
            /* Currently doesn't currently support ARRAY of ARRAY, so try another type
             * if this happens. Also check for too much recursion. */
            if ((H5T_ARRAY == parent_class) || ((depth + 1) > TYPE_GEN_RECURSION_MAX_DEPTH))
                goto roll_datatype;

            gen_func = generate_random_datatype_array;
            break;
        default:
            printf("    invalid datatype class\n");
            goto done;
            break;
    }

    if ((datatype = (gen_func)(parent_class, is_compact)) < 0) {
        printf("    couldn't generate datatype\n");
        goto done;
    }

    /*
     * Check to make sure that the generated datatype does
     * not exceed the maximum datatype size and doesn't exceed
     * the maximum compact datatype size if a compact datatype
     * was requested.
     */
    if (depth == 1) {
        if (0 == (type_size = H5Tget_size(datatype))) {
            printf("    failed to retrieve datatype's size\n");
            H5Tclose(datatype);
            datatype = H5I_INVALID_HID;
            goto done;
        }

        if ((type_size > GENERATED_DATATYPE_MAX_SIZE) ||
            (is_compact && (type_size > COMPACT_DATATYPE_MAX_SIZE))) {
            /*
             * Generate a new datatype.
             */
            H5Tclose(datatype);
            datatype = H5I_INVALID_HID;
            goto roll_datatype;
        }
    }

    ret_value = datatype;

done:
    depth--;

    return ret_value;
}

static hid_t
generate_random_datatype_integer(H5T_class_t H5_ATTR_UNUSED parent_class, bool H5_ATTR_UNUSED is_compact)
{
    hid_t type_to_copy = H5I_INVALID_HID;
    hid_t datatype     = H5I_INVALID_HID;
    hid_t ret_value    = H5I_INVALID_HID;

    switch (rand() % NUM_PREDEFINED_INT_TYPES) {
        case 0:
            type_to_copy = H5T_STD_I8BE;
            break;
        case 1:
            type_to_copy = H5T_STD_I8LE;
            break;
        case 2:
            type_to_copy = H5T_STD_I16BE;
            break;
        case 3:
            type_to_copy = H5T_STD_I16LE;
            break;
        case 4:
            type_to_copy = H5T_STD_I32BE;
            break;
        case 5:
            type_to_copy = H5T_STD_I32LE;
            break;
        case 6:
            type_to_copy = H5T_STD_I64BE;
            break;
        case 7:
            type_to_copy = H5T_STD_I64LE;
            break;
        case 8:
            type_to_copy = H5T_STD_U8BE;
            break;
        case 9:
            type_to_copy = H5T_STD_U8LE;
            break;
        case 10:
            type_to_copy = H5T_STD_U16BE;
            break;
        case 11:
            type_to_copy = H5T_STD_U16LE;
            break;
        case 12:
            type_to_copy = H5T_STD_U32BE;
            break;
        case 13:
            type_to_copy = H5T_STD_U32LE;
            break;
        case 14:
            type_to_copy = H5T_STD_U64BE;
            break;
        case 15:
            type_to_copy = H5T_STD_U64LE;
            break;
        default:
            printf("    invalid value for predefined integer type; should not happen\n");
            goto done;
    }

    if ((datatype = H5Tcopy(type_to_copy)) < 0) {
        printf("    couldn't copy predefined integer type\n");
        goto done;
    }

    ret_value = datatype;

done:
    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_float(H5T_class_t H5_ATTR_UNUSED parent_class, bool H5_ATTR_UNUSED is_compact)
{
    hid_t type_to_copy = H5I_INVALID_HID;
    hid_t datatype     = H5I_INVALID_HID;
    hid_t ret_value    = H5I_INVALID_HID;

    switch (rand() % NUM_PREDEFINED_FLOAT_TYPES) {
        case 0:
            type_to_copy = H5T_IEEE_F32BE;
            break;
        case 1:
            type_to_copy = H5T_IEEE_F32LE;
            break;
        case 2:
            type_to_copy = H5T_IEEE_F64BE;
            break;
        case 3:
            type_to_copy = H5T_IEEE_F64LE;
            break;

        default:
            printf("    invalid value for floating point type; should not happen\n");
            goto done;
    }

    if ((datatype = H5Tcopy(type_to_copy)) < 0) {
        printf("    couldn't copy predefined floating-point type\n");
        goto done;
    }

    ret_value = datatype;

done:
    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_string(H5T_class_t H5_ATTR_UNUSED parent_class, bool H5_ATTR_UNUSED is_compact)
{
    hid_t datatype  = H5I_INVALID_HID;
    hid_t ret_value = H5I_INVALID_HID;

    /* Note: currently only H5T_CSET_ASCII is supported for the character set and
     * only H5T_STR_NULLTERM is supported for string padding for variable-length
     * strings and only H5T_STR_NULLPAD is supported for string padding for
     * fixed-length strings, but these may change in the future.
     */
#if 0 /* Currently, all VL types are disabled */
    if (0 == (rand() % 2)) {
#endif
    if ((datatype = H5Tcreate(H5T_STRING, (size_t)(rand() % STRING_TYPE_MAX_SIZE) + 1)) < 0) {
        printf("    couldn't create fixed-length string datatype\n");
        goto done;
    }

    if (H5Tset_strpad(datatype, H5T_STR_NULLPAD) < 0) {
        printf("    couldn't set H5T_STR_NULLPAD for fixed-length string type\n");
        goto done;
    }
#if 0 /* Currently, all VL types are disabled */
    }
    else {

            if ((datatype = H5Tcreate(H5T_STRING, H5T_VARIABLE)) < 0) {
                H5_FAILED();
                printf("    couldn't create variable-length string datatype\n");
                goto done;
            }

            if (H5Tset_strpad(datatype, H5T_STR_NULLTERM) < 0) {
                H5_FAILED();
                printf("    couldn't set H5T_STR_NULLTERM for variable-length string type\n");
                goto done;
            }
    }
#endif

    if (H5Tset_cset(datatype, H5T_CSET_ASCII) < 0) {
        printf("    couldn't set string datatype character set\n");
        goto done;
    }

    ret_value = datatype;

done:
    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_compound(H5T_class_t H5_ATTR_UNUSED parent_class, bool is_compact)
{
    size_t num_members   = 0;
    size_t next_offset   = 0;
    size_t compound_size = 0;
    hid_t  compound_members[COMPOUND_TYPE_MAX_MEMBERS];
    hid_t  datatype  = H5I_INVALID_HID;
    hid_t  ret_value = H5I_INVALID_HID;

    for (size_t i = 0; i < COMPOUND_TYPE_MAX_MEMBERS; i++)
        compound_members[i] = H5I_INVALID_HID;

    if ((datatype = H5Tcreate(H5T_COMPOUND, 1)) < 0) {
        printf("    couldn't create compound datatype\n");
        goto done;
    }

    num_members = (size_t)(rand() % COMPOUND_TYPE_MAX_MEMBERS + 1);

    for (size_t i = 0; i < num_members; i++) {
        size_t member_size;
        char   member_name[256];

        snprintf(member_name, 256, "compound_member%zu", i);

        if ((compound_members[i] = generate_random_datatype(H5T_COMPOUND, is_compact)) < 0) {
            printf("    couldn't create compound datatype member %zu\n", i);
            goto done;
        }

        if (!(member_size = H5Tget_size(compound_members[i]))) {
            printf("    couldn't get compound member %zu size\n", i);
            goto done;
        }

        compound_size += member_size;

        if (H5Tset_size(datatype, compound_size) < 0) {
            printf("    couldn't set size for compound datatype\n");
            goto done;
        }

        if (H5Tinsert(datatype, member_name, next_offset, compound_members[i]) < 0) {
            printf("    couldn't insert compound datatype member %zu\n", i);
            goto done;
        }

        next_offset += member_size;
    }

    ret_value = datatype;

done:
    for (size_t i = 0; i < COMPOUND_TYPE_MAX_MEMBERS; i++) {
        if (compound_members[i] > 0 && H5Tclose(compound_members[i]) < 0) {
            printf("    couldn't close compound member %zu\n", i);
        }
    }

    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_reference(H5T_class_t H5_ATTR_UNUSED parent_class, bool H5_ATTR_UNUSED is_compact)
{
    hid_t datatype  = H5I_INVALID_HID;
    hid_t ret_value = H5I_INVALID_HID;

#if 0 /* Region references are currently unsupported */
    if (0 == (rand() % 2)) {
#endif
    if ((datatype = H5Tcopy(H5T_STD_REF_OBJ)) < 0) {
        printf("    couldn't copy object reference datatype\n");
        goto done;
    }
#if 0 /* Region references are currently unsupported */
    }
    else {
        if ((datatype = H5Tcopy(H5T_STD_REF_DSETREG)) < 0) {
            H5_FAILED();
            printf("    couldn't copy region reference datatype\n");
            goto done;
        }
    }
#endif

    ret_value = datatype;

done:
    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_enum(H5T_class_t H5_ATTR_UNUSED parent_class, bool H5_ATTR_UNUSED is_compact)
{
    size_t num_members      = 0;
    hid_t  datatype         = H5I_INVALID_HID;
    int   *enum_member_vals = NULL;
    hid_t  ret_value        = H5I_INVALID_HID;

    if ((datatype = H5Tenum_create(H5T_NATIVE_INT)) < 0) {
        printf("    couldn't create enum datatype\n");
        goto done;
    }

    num_members = (size_t)(rand() % ENUM_TYPE_MAX_MEMBERS + 1);

    if (NULL == (enum_member_vals = malloc(num_members * sizeof(int)))) {
        printf("    couldn't allocate space for enum members\n");
        goto done;
    }

    for (size_t i = 0; i < num_members; i++) {
        bool unique;
        char name[ENUM_TYPE_MAX_MEMBER_NAME_LENGTH];
        int  enum_val;

        snprintf(name, ENUM_TYPE_MAX_MEMBER_NAME_LENGTH, "enum_val%zu", i);

        do {
            enum_val = rand();

            /* Check for uniqueness of enum member */
            unique = true;
            for (size_t j = 0; j < i; j++)
                if (enum_val == enum_member_vals[j])
                    unique = false;
        } while (!unique);

        enum_member_vals[i] = enum_val;

        if (H5Tenum_insert(datatype, name, &enum_val) < 0) {
            printf("    couldn't insert member into enum datatype\n");
            goto done;
        }
    }

    ret_value = datatype;

done:
    free(enum_member_vals);

    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

static hid_t
generate_random_datatype_array(H5T_class_t H5_ATTR_UNUSED parent_class, bool is_compact)
{
    unsigned ndims         = 0;
    hsize_t *array_dims    = NULL;
    hid_t    base_datatype = H5I_INVALID_HID;
    hid_t    datatype      = H5I_INVALID_HID;
    hid_t    ret_value     = H5I_INVALID_HID;

    ndims = (unsigned)(rand() % ARRAY_TYPE_MAX_DIMS + 1);

    if (NULL == (array_dims = malloc(ndims * sizeof(*array_dims)))) {
        printf("    couldn't allocate space for array datatype dims\n");
        goto done;
    }

    for (size_t i = 0; i < ndims; i++)
        array_dims[i] = (hsize_t)(rand() % MAX_DIM_SIZE + 1);

    if ((base_datatype = generate_random_datatype(H5T_ARRAY, is_compact)) < 0) {
        printf("    couldn't create array base datatype\n");
        goto done;
    }

    if ((datatype = H5Tarray_create2(base_datatype, ndims, array_dims)) < 0) {
        printf("    couldn't create array datatype\n");
        goto done;
    }

    ret_value = datatype;

done:
    free(array_dims);

    if (base_datatype >= 0) {
        if (H5Tclose(base_datatype) < 0)
            printf("    couldn't close array base datatype\n");
    }

    if ((ret_value == H5I_INVALID_HID) && (datatype >= 0)) {
        if (H5Tclose(datatype) < 0)
            printf("    couldn't close datatype\n");
    }

    return ret_value;
}

/*
 * Helper function to generate a random HDF5 dataspace in order to thoroughly
 * test support for dataspaces.
 */
hid_t
generate_random_dataspace(int rank, const hsize_t *max_dims, hsize_t *dims_out, bool is_compact)
{
    hsize_t dataspace_dims[H5S_MAX_RANK];
    size_t  i;
    hid_t   dataspace_id = H5I_INVALID_HID;

    if (rank < 0)
        TEST_ERROR;
    if (is_compact && (rank > COMPACT_SPACE_MAX_DIMS)) {
        printf("    current rank of compact dataspace (%lld) exceeds maximum dimensionality (%lld)\n",
               (long long)rank, (long long)COMPACT_SPACE_MAX_DIMS);
        TEST_ERROR;
    }

    /*
     * XXX: if max_dims is specified, make sure that the dimensions generated
     * are not larger than this.
     */
    for (i = 0; i < (size_t)rank; i++) {
        if (is_compact)
            dataspace_dims[i] = (hsize_t)(rand() % COMPACT_SPACE_MAX_DIM_SIZE + 1);
        else
            dataspace_dims[i] = (hsize_t)(rand() % MAX_DIM_SIZE + 1);

        if (dims_out)
            dims_out[i] = dataspace_dims[i];
    }

    if ((dataspace_id = H5Screate_simple(rank, dataspace_dims, max_dims)) < 0)
        TEST_ERROR;

    return dataspace_id;

error:
    return H5I_INVALID_HID;
}

int
create_test_container(char *filename, uint64_t vol_cap_flags)
{
    hid_t file_id  = H5I_INVALID_HID;
    hid_t group_id = H5I_INVALID_HID;

    if (!(vol_cap_flags & H5VL_CAP_FLAG_FILE_BASIC)) {
        printf("   VOL connector doesn't support file creation\n");
        goto error;
    }

    if ((file_id = H5Fcreate(filename, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT)) < 0) {
        printf("    couldn't create testing container file '%s'\n", filename);
        goto error;
    }

    if (vol_cap_flags & H5VL_CAP_FLAG_GROUP_BASIC) {
        /* Create container groups for each of the test interfaces
         * (group, attribute, dataset, etc.).
         */
        if ((group_id = H5Gcreate2(file_id, GROUP_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) >=
            0) {
            printf("    created container group for Group tests\n");
            H5Gclose(group_id);
        }

        if ((group_id = H5Gcreate2(file_id, ATTRIBUTE_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT,
                                   H5P_DEFAULT)) >= 0) {
            printf("    created container group for Attribute tests\n");
            H5Gclose(group_id);
        }

        if ((group_id =
                 H5Gcreate2(file_id, DATASET_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) >= 0) {
            printf("    created container group for Dataset tests\n");
            H5Gclose(group_id);
        }

        if ((group_id =
                 H5Gcreate2(file_id, DATATYPE_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) >= 0) {
            printf("    created container group for Datatype tests\n");
            H5Gclose(group_id);
        }

        if ((group_id = H5Gcreate2(file_id, LINK_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) >=
            0) {
            printf("    created container group for Link tests\n");
            H5Gclose(group_id);
        }

        if ((group_id = H5Gcreate2(file_id, OBJECT_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT)) >=
            0) {
            printf("    created container group for Object tests\n");
            H5Gclose(group_id);
        }

        if ((group_id = H5Gcreate2(file_id, MISCELLANEOUS_TEST_GROUP_NAME, H5P_DEFAULT, H5P_DEFAULT,
                                   H5P_DEFAULT)) >= 0) {
            printf("    created container group for Miscellaneous tests\n");
            H5Gclose(group_id);
        }
    }

    if (H5Fclose(file_id) < 0) {
        printf("    failed to close testing container\n");
        goto error;
    }

    return 0;

error:
    H5E_BEGIN_TRY
    {
        H5Gclose(group_id);
        H5Fclose(file_id);
    }
    H5E_END_TRY

    return -1;
}

/*
 * Add a prefix to the given filename. The caller
 * is responsible for freeing the returned filename
 * pointer with free().
 */
herr_t
prefix_filename(const char *prefix, const char *filename, char **filename_out)
{
    char  *out_buf   = NULL;
    herr_t ret_value = SUCCEED;

    if (!prefix) {
        printf("    invalid file prefix\n");
        ret_value = FAIL;
        goto done;
    }
    if (!filename || (*filename == '\0')) {
        printf("    invalid filename\n");
        ret_value = FAIL;
        goto done;
    }
    if (!filename_out) {
        printf("    invalid filename_out buffer\n");
        ret_value = FAIL;
        goto done;
    }

    if (NULL == (out_buf = malloc(H5_API_TEST_FILENAME_MAX_LENGTH))) {
        printf("    couldn't allocated filename buffer\n");
        ret_value = FAIL;
        goto done;
    }

    snprintf(out_buf, H5_API_TEST_FILENAME_MAX_LENGTH, "%s%s", prefix, filename);

    *filename_out = out_buf;

done:
    return ret_value;
}

/*
 * Calls H5Fdelete on the given filename. If a prefix string
 * is given, adds that prefix string to the filename before
 * calling H5Fdelete
 */
herr_t
remove_test_file(const char *prefix, const char *filename)
{
    const char *test_file;
    char       *prefixed_filename = NULL;
    herr_t      ret_value         = SUCCEED;

    if (prefix) {
        if (prefix_filename(prefix, filename, &prefixed_filename) < 0) {
            printf("    couldn't prefix filename\n");
            ret_value = FAIL;
            goto done;
        }

        test_file = prefixed_filename;
    }
    else
        test_file = filename;

    if (H5Fdelete(test_file, H5P_DEFAULT) < 0) {
        printf("    couldn't remove file '%s'\n", test_file);
        ret_value = FAIL;
        goto done;
    }

done:
    free(prefixed_filename);

    return ret_value;
}
