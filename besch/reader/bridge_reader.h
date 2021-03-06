#ifndef __BRIDGE_READER_H
#define __BRIDGE_READER_H

#include "obj_reader.h"

class bridge_reader_t : public obj_reader_t {
    static bridge_reader_t the_instance;

    bridge_reader_t() { register_reader(); }
protected:
    virtual void register_obj(obj_besch_t *&data);
    virtual bool successfully_loaded() const;

public:
    static bridge_reader_t*instance() { return &the_instance; }

    /**
     * Read a goods info node. Does version check and
     * compatibility transformations.
     * @author Hj. Malthaner
     */
    obj_besch_t * read_node(FILE *fp, obj_node_info_t &node);

    virtual obj_type get_type() const { return obj_bridge; }
    virtual const char *get_type_name() const { return "bridge"; }
};

#endif
