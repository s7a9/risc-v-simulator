#include "RvALU.h"

void ResrvStation::execute(ComnDataBus* cdb, size_t cdbn) {
	if (state == RS_Waiting) {
		bool f1 = Q1, f2 = Q2;
		for (int i = 0; i < cdbn; ++i) if (cdb[i].busy) {
			if (f1 && Q1 == cdb[i].Q) {
				V1 = cdb[i].val;
				Q1 = 0;
				f1 = false;
			}
			if (f2 && Q2 == cdb[i].Q) {
				V2 = cdb[i].val;
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

void ALU::execute(ComnDataBus* cdb) {
	if (!cdb) return;
	if (rs.state == RS_Ready) {
		uint32 uv1 = rs.V1, uv2 = rs.V2;
		cdb->busy = true;
		cdb->Q = rs.Dst;
		rs.state = RS_Committed;
		switch (rs.Op) {
		case 0:
			if ((rs.cmd & 127) == Instr_ALUI) {
				cdb->val = rs.V1 + rs.V2;
			}
			else {
				cdb->val = (rs.cmd & 0x40000000) ?
					rs.V1 - rs.V2 : rs.V1 + rs.V2;
			}
			break;
		case 1: cdb->val = rs.V1 << (rs.V2 & 31); break;
		case 2: cdb->val = (rs.V1 < rs.V2) ? 1 : 0; break;
		case 3: cdb->val = (uv1 < uv2) ? 1 : 0; break;
		case 4: cdb->val = uv1 ^ uv2; break;
		case 5:
			if ((rs.cmd & 0x40000000) && (uv1 & 0x80000000)) {
				uv2 &= 31;
				cdb->val = uv1 >> uv2;
				while (uv2--) {
					cdb->val |= 1u << (31 - uv2);
				}
			}
			else cdb->val = uv1 >> uv2;
			break;
		case 6: cdb->val = uv1 | uv2; break;
		case 7: cdb->val = uv1 & uv2; break;
		}
	}
}