设昨日最高价：H
昨日收盘价：C
昨日最低价：L
P = (H+C+L)/3

B_break = H+2×（P-L）
S_setup = P+H-L
S_enter = 2×P-L

B_enter = 2×P-H
B_setup = P-H+L
S_break = L-2×（H-P）

开仓：	最新价超过B_break	开多
			最新价跌破S_break		开空
			
持多仓：最新价从最高点回落到S_setup，继续回落到S_enter，则平仓并反向开仓
持空仓：最新价从最低点上升到B_setup，继续上升到B_enter，则平仓并反向开仓