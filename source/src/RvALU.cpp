#include "RvALU.h"

void ResrvStation::write_result_(const std::vector<ComnDataBus>& cdbs) {
	if (state == RS_Waiting) {
		bool f1 = Q1, f2 = Q2;
		for (auto& cdb : cdbs)
			if (cdb.busy) {
				if (f1 && Q1 == cdb.Q) {
					V1 = cdb.val;
					Q1 = 0;
					f1 = false;
				}
				if (f2 && Q2 == cdb.Q) {
					V2 = cdb.val;
					Q2 = 0;
					f2 = false;
				}
			}
		if (!(f1 || f2)) state = RS_Ready;
	}
}

void ResrvStation::tick() {
	Op.tick(); Dst.tick(); state.tick();
	Q1.tick(); Q2.tick();
	V1.tick(); V2.tick();
	cmd.tick();
}

#include <cstdio>

void ALU::execute(ComnDataBus* cdb, ResrvStation& rs) {
	if (!cdb) return;
	if (rs.state == RS_Ready) {
		uint32 uv1 = rs.V1, uv2 = rs.V2;
		cdb->busy = true;
		cdb->Q = rs.Dst;
		rs.state = RS_Idle;
		if ((rs.cmd & 127) == Instr_Branch) {
			switch (rs.Op) {
			case 0: cdb->val = rs.V1 == rs.V2; break;
			case 1: cdb->val = rs.V1 != rs.V2; break;
			case 4: cdb->val = rs.V1 <  rs.V2; break;
			case 5: cdb->val = rs.V1 >= rs.V2; break;
			case 6: cdb->val = uv1 <  uv2; break;
			case 7: cdb->val = uv1 >= uv2; break;
			}
			return;
		}
		// DEBUG!! //
		/*
		if (rs.cmd == 0x00f54533) {
			printf("%u", rs.V1.cur_data);
		}
		if (rs.cmd == 0x00d7c7b3) {
			printf("%c", rs.V1.cur_data & 255);
		}*/
		// ------- //
		switch (rs.Op) {
		case 0:
			if ((rs.cmd & 127) == Instr_ALU) {
				cdb->val = (rs.cmd & 0x40000000) ?
					rs.V1 - rs.V2 : rs.V1 + rs.V2;
			}
			else cdb->val = rs.V1 + rs.V2;
			break;
		case 1: cdb->val = rs.V1 << (rs.V2 & 31); break;
		case 2: cdb->val = (rs.V1 < rs.V2) ? 1 : 0; break;
		case 3: cdb->val = (uv1 < uv2) ? 1 : 0; break;
		case 4: cdb->val = uv1 ^ uv2; break;
		case 5:
			uv2 &= 31; 
			cdb->val = uv1 >> uv2;
			if ((rs.cmd & 0x40000000) && (uv1 & 0x80000000))
				cdb->val |= 0xFFFFFFFF << (32 - uv2);
			break;
		case 6: cdb->val = uv1 | uv2; break;
		case 7: cdb->val = uv1 & uv2; break;
		}
	}
}