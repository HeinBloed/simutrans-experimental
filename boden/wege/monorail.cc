#include "../../simtypes.h"
#include "../../simdebug.h"
#include "../grund.h"
#include "../../bauer/wegbauer.h"
#include "../../besch/weg_besch.h"

#include "monorail.h"

const weg_besch_t *monorail_t::default_monorail=NULL;



monorail_t::monorail_t(karte_t *welt, loadsave_t *file) : schiene_t(welt)
{
	rdwr(file);
}



void monorail_t::rdwr(loadsave_t *file)
{
	schiene_t::rdwr(file);

	if(get_besch()->get_wtyp()!=monorail_wt) {
		int old_max_speed = get_max_speed();
		int old_max_weight = get_max_weight();
		const weg_besch_t *besch = wegbauer_t::weg_search( monorail_wt, (old_max_speed>0 ? old_max_speed : 120), (old_max_weight > 0 ? old_max_weight : 10), 0, (weg_t::system_type)((get_besch()->get_styp()==weg_t::type_elevated)*weg_t::type_elevated) );
		dbg->warning("monorail_t::rwdr()", "Unknown way replaced by monorail %s (old_max_speed %i)", besch->get_name(), old_max_speed );
		set_besch(besch);
		if(old_max_speed>0) {
			set_max_speed(old_max_speed);
		}
		if(old_max_weight > 0)
		{
			set_max_weight(old_max_weight);
		}
	}
}
