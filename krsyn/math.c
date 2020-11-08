#include "math.h"


static const int16_t sin_table[ks_1(KS_TABLE_BITS)] = {
    0, 201, 402, 603, 804, 1005, 1206, 1406,
    1607, 1808, 2009, 2209, 2410, 2610, 2811, 3011,
    3211, 3411, 3611, 3811, 4011, 4210, 4409, 4608,
    4807, 5006, 5205, 5403, 5601, 5799, 5997, 6195,
    6392, 6589, 6786, 6982, 7179, 7375, 7571, 7766,
    7961, 8156, 8351, 8545, 8739, 8932, 9126, 9319,
    9511, 9703, 9895, 10087, 10278, 10469, 10659, 10849,
    11038, 11227, 11416, 11604, 11792, 11980, 12166, 12353,
    12539, 12724, 12909, 13094, 13278, 13462, 13645, 13827,
    14009, 14191, 14372, 14552, 14732, 14911, 15090, 15268,
    15446, 15623, 15799, 15975, 16150, 16325, 16499, 16672,
    16845, 17017, 17189, 17360, 17530, 17699, 17868, 18036,
    18204, 18371, 18537, 18702, 18867, 19031, 19194, 19357,
    19519, 19680, 19840, 20000, 20159, 20317, 20474, 20631,
    20787, 20942, 21096, 21249, 21402, 21554, 21705, 21855,
    22004, 22153, 22301, 22448, 22594, 22739, 22883, 23027,
    23169, 23311, 23452, 23592, 23731, 23869, 24006, 24143,
    24278, 24413, 24546, 24679, 24811, 24942, 25072, 25201,
    25329, 25456, 25582, 25707, 25831, 25954, 26077, 26198,
    26318, 26437, 26556, 26673, 26789, 26905, 27019, 27132,
    27244, 27355, 27466, 27575, 27683, 27790, 27896, 28001,
    28105, 28208, 28309, 28410, 28510, 28608, 28706, 28802,
    28897, 28992, 29085, 29177, 29268, 29358, 29446, 29534,
    29621, 29706, 29790, 29873, 29955, 30036, 30116, 30195,
    30272, 30349, 30424, 30498, 30571, 30643, 30713, 30783,
    30851, 30918, 30984, 31049, 31113, 31175, 31236, 31297,
    31356, 31413, 31470, 31525, 31580, 31633, 31684, 31735,
    31785, 31833, 31880, 31926, 31970, 32014, 32056, 32097,
    32137, 32176, 32213, 32249, 32284, 32318, 32350, 32382,
    32412, 32441, 32468, 32495, 32520, 32544, 32567, 32588,
    32609, 32628, 32646, 32662, 32678, 32692, 32705, 32717,
    32727, 32736, 32744, 32751, 32757, 32761, 32764, 32766,
    32767, 32766, 32764, 32761, 32757, 32751, 32744, 32736,
    32727, 32717, 32705, 32692, 32678, 32662, 32646, 32628,
    32609, 32588, 32567, 32544, 32520, 32495, 32468, 32441,
    32412, 32382, 32350, 32318, 32284, 32249, 32213, 32176,
    32137, 32097, 32056, 32014, 31970, 31926, 31880, 31833,
    31785, 31735, 31684, 31633, 31580, 31525, 31470, 31413,
    31356, 31297, 31236, 31175, 31113, 31049, 30984, 30918,
    30851, 30783, 30713, 30643, 30571, 30498, 30424, 30349,
    30272, 30195, 30116, 30036, 29955, 29873, 29790, 29706,
    29621, 29534, 29446, 29358, 29268, 29177, 29085, 28992,
    28897, 28802, 28706, 28608, 28510, 28410, 28309, 28208,
    28105, 28001, 27896, 27790, 27683, 27575, 27466, 27355,
    27244, 27132, 27019, 26905, 26789, 26673, 26556, 26437,
    26318, 26198, 26077, 25954, 25831, 25707, 25582, 25456,
    25329, 25201, 25072, 24942, 24811, 24679, 24546, 24413,
    24278, 24143, 24006, 23869, 23731, 23592, 23452, 23311,
    23169, 23027, 22883, 22739, 22594, 22448, 22301, 22153,
    22004, 21855, 21705, 21554, 21402, 21249, 21096, 20942,
    20787, 20631, 20474, 20317, 20159, 20000, 19840, 19680,
    19519, 19357, 19194, 19031, 18867, 18702, 18537, 18371,
    18204, 18036, 17868, 17699, 17530, 17360, 17189, 17017,
    16845, 16672, 16499, 16325, 16150, 15975, 15799, 15623,
    15446, 15268, 15090, 14911, 14732, 14552, 14372, 14191,
    14009, 13827, 13645, 13462, 13278, 13094, 12909, 12724,
    12539, 12353, 12166, 11980, 11792, 11604, 11416, 11227,
    11038, 10849, 10659, 10469, 10278, 10087, 9895, 9703,
    9511, 9319, 9126, 8932, 8739, 8545, 8351, 8156,
    7961, 7766, 7571, 7375, 7179, 6982, 6786, 6589,
    6392, 6195, 5997, 5799, 5601, 5403, 5205, 5006,
    4807, 4608, 4409, 4210, 4011, 3811, 3611, 3411,
    3211, 3011, 2811, 2610, 2410, 2209, 2009, 1808,
    1607, 1406, 1206, 1005, 804, 603, 402, 201,
    0, -201, -402, -603, -804, -1005, -1206, -1406,
    -1607, -1808, -2009, -2209, -2410, -2610, -2811, -3011,
    -3211, -3411, -3611, -3811, -4011, -4210, -4409, -4608,
    -4807, -5006, -5205, -5403, -5601, -5799, -5997, -6195,
    -6392, -6589, -6786, -6982, -7179, -7375, -7571, -7766,
    -7961, -8156, -8351, -8545, -8739, -8932, -9126, -9319,
    -9511, -9703, -9895, -10087, -10278, -10469, -10659, -10849,
    -11038, -11227, -11416, -11604, -11792, -11980, -12166, -12353,
    -12539, -12724, -12909, -13094, -13278, -13462, -13645, -13827,
    -14009, -14191, -14372, -14552, -14732, -14911, -15090, -15268,
    -15446, -15623, -15799, -15975, -16150, -16325, -16499, -16672,
    -16845, -17017, -17189, -17360, -17530, -17699, -17868, -18036,
    -18204, -18371, -18537, -18702, -18867, -19031, -19194, -19357,
    -19519, -19680, -19840, -20000, -20159, -20317, -20474, -20631,
    -20787, -20942, -21096, -21249, -21402, -21554, -21705, -21855,
    -22004, -22153, -22301, -22448, -22594, -22739, -22883, -23027,
    -23169, -23311, -23452, -23592, -23731, -23869, -24006, -24143,
    -24278, -24413, -24546, -24679, -24811, -24942, -25072, -25201,
    -25329, -25456, -25582, -25707, -25831, -25954, -26077, -26198,
    -26318, -26437, -26556, -26673, -26789, -26905, -27019, -27132,
    -27244, -27355, -27466, -27575, -27683, -27790, -27896, -28001,
    -28105, -28208, -28309, -28410, -28510, -28608, -28706, -28802,
    -28897, -28992, -29085, -29177, -29268, -29358, -29446, -29534,
    -29621, -29706, -29790, -29873, -29955, -30036, -30116, -30195,
    -30272, -30349, -30424, -30498, -30571, -30643, -30713, -30783,
    -30851, -30918, -30984, -31049, -31113, -31175, -31236, -31297,
    -31356, -31413, -31470, -31525, -31580, -31633, -31684, -31735,
    -31785, -31833, -31880, -31926, -31970, -32014, -32056, -32097,
    -32137, -32176, -32213, -32249, -32284, -32318, -32350, -32382,
    -32412, -32441, -32468, -32495, -32520, -32544, -32567, -32588,
    -32609, -32628, -32646, -32662, -32678, -32692, -32705, -32717,
    -32727, -32736, -32744, -32751, -32757, -32761, -32764, -32766,
    -32767, -32766, -32764, -32761, -32757, -32751, -32744, -32736,
    -32727, -32717, -32705, -32692, -32678, -32662, -32646, -32628,
    -32609, -32588, -32567, -32544, -32520, -32495, -32468, -32441,
    -32412, -32382, -32350, -32318, -32284, -32249, -32213, -32176,
    -32137, -32097, -32056, -32014, -31970, -31926, -31880, -31833,
    -31785, -31735, -31684, -31633, -31580, -31525, -31470, -31413,
    -31356, -31297, -31236, -31175, -31113, -31049, -30984, -30918,
    -30851, -30783, -30713, -30643, -30571, -30498, -30424, -30349,
    -30272, -30195, -30116, -30036, -29955, -29873, -29790, -29706,
    -29621, -29534, -29446, -29358, -29268, -29177, -29085, -28992,
    -28897, -28802, -28706, -28608, -28510, -28410, -28309, -28208,
    -28105, -28001, -27896, -27790, -27683, -27575, -27466, -27355,
    -27244, -27132, -27019, -26905, -26789, -26673, -26556, -26437,
    -26318, -26198, -26077, -25954, -25831, -25707, -25582, -25456,
    -25329, -25201, -25072, -24942, -24811, -24679, -24546, -24413,
    -24278, -24143, -24006, -23869, -23731, -23592, -23452, -23311,
    -23169, -23027, -22883, -22739, -22594, -22448, -22301, -22153,
    -22004, -21855, -21705, -21554, -21402, -21249, -21096, -20942,
    -20787, -20631, -20474, -20317, -20159, -20000, -19840, -19680,
    -19519, -19357, -19194, -19031, -18867, -18702, -18537, -18371,
    -18204, -18036, -17868, -17699, -17530, -17360, -17189, -17017,
    -16845, -16672, -16499, -16325, -16150, -15975, -15799, -15623,
    -15446, -15268, -15090, -14911, -14732, -14552, -14372, -14191,
    -14009, -13827, -13645, -13462, -13278, -13094, -12909, -12724,
    -12539, -12353, -12166, -11980, -11792, -11604, -11416, -11227,
    -11038, -10849, -10659, -10469, -10278, -10087, -9895, -9703,
    -9511, -9319, -9126, -8932, -8739, -8545, -8351, -8156,
    -7961, -7766, -7571, -7375, -7179, -6982, -6786, -6589,
    -6392, -6195, -5997, -5799, -5601, -5403, -5205, -5006,
    -4807, -4608, -4409, -4210, -4011, -3811, -3611, -3411,
    -3211, -3011, -2811, -2610, -2410, -2209, -2009, -1808,
    -1607, -1406, -1206, -1005, -804, -603, -402, -201,
};
static const int16_t saw_table[ks_1(KS_TABLE_BITS)] = {
    32767, 32704, 32640, 32576, 32512, 32448, 32384, 32320,
    32256, 32192, 32128, 32064, 32000, 31936, 31872, 31808,
    31744, 31680, 31616, 31552, 31488, 31424, 31360, 31296,
    31232, 31168, 31104, 31040, 30976, 30912, 30848, 30784,
    30720, 30656, 30592, 30528, 30464, 30400, 30336, 30272,
    30208, 30144, 30080, 30016, 29952, 29888, 29824, 29760,
    29696, 29632, 29568, 29504, 29440, 29376, 29312, 29248,
    29184, 29120, 29056, 28992, 28928, 28864, 28800, 28736,
    28672, 28608, 28544, 28480, 28416, 28352, 28288, 28224,
    28160, 28096, 28032, 27968, 27904, 27840, 27776, 27712,
    27648, 27584, 27520, 27456, 27392, 27328, 27264, 27200,
    27136, 27072, 27008, 26944, 26880, 26816, 26752, 26688,
    26624, 26560, 26496, 26432, 26368, 26304, 26240, 26176,
    26112, 26048, 25984, 25920, 25856, 25792, 25728, 25664,
    25600, 25536, 25472, 25408, 25344, 25280, 25216, 25152,
    25088, 25024, 24960, 24896, 24832, 24768, 24704, 24640,
    24576, 24512, 24448, 24384, 24320, 24256, 24192, 24128,
    24064, 24000, 23936, 23872, 23808, 23744, 23680, 23616,
    23552, 23488, 23424, 23360, 23296, 23232, 23168, 23104,
    23040, 22976, 22912, 22848, 22784, 22720, 22656, 22592,
    22528, 22464, 22400, 22336, 22272, 22208, 22144, 22080,
    22016, 21952, 21888, 21824, 21760, 21696, 21632, 21568,
    21504, 21440, 21376, 21312, 21248, 21184, 21120, 21056,
    20992, 20928, 20864, 20800, 20736, 20672, 20608, 20544,
    20480, 20416, 20352, 20288, 20224, 20160, 20096, 20032,
    19968, 19904, 19840, 19776, 19712, 19648, 19584, 19520,
    19456, 19392, 19328, 19264, 19200, 19136, 19072, 19008,
    18944, 18880, 18816, 18752, 18688, 18624, 18560, 18496,
    18432, 18368, 18304, 18240, 18176, 18112, 18048, 17984,
    17920, 17856, 17792, 17728, 17664, 17600, 17536, 17472,
    17408, 17344, 17280, 17216, 17152, 17088, 17024, 16960,
    16896, 16832, 16768, 16704, 16640, 16576, 16512, 16448,
    16384, 16320, 16256, 16192, 16128, 16064, 16000, 15936,
    15872, 15808, 15744, 15680, 15616, 15552, 15488, 15424,
    15360, 15296, 15232, 15168, 15104, 15040, 14976, 14912,
    14848, 14784, 14720, 14656, 14592, 14528, 14464, 14400,
    14336, 14272, 14208, 14144, 14080, 14016, 13952, 13888,
    13824, 13760, 13696, 13632, 13568, 13504, 13440, 13376,
    13312, 13248, 13184, 13120, 13056, 12992, 12928, 12864,
    12800, 12736, 12672, 12608, 12544, 12480, 12416, 12352,
    12288, 12224, 12160, 12096, 12032, 11968, 11904, 11840,
    11776, 11712, 11648, 11584, 11520, 11456, 11392, 11328,
    11264, 11200, 11136, 11072, 11008, 10944, 10880, 10816,
    10752, 10688, 10624, 10560, 10496, 10432, 10368, 10304,
    10240, 10176, 10112, 10048, 9984, 9920, 9856, 9792,
    9728, 9664, 9600, 9536, 9472, 9408, 9344, 9280,
    9216, 9152, 9088, 9024, 8960, 8896, 8832, 8768,
    8704, 8640, 8576, 8512, 8448, 8384, 8320, 8256,
    8192, 8128, 8064, 8000, 7936, 7872, 7808, 7744,
    7680, 7616, 7552, 7488, 7424, 7360, 7296, 7232,
    7168, 7104, 7040, 6976, 6912, 6848, 6784, 6720,
    6656, 6592, 6528, 6464, 6400, 6336, 6272, 6208,
    6144, 6080, 6016, 5952, 5888, 5824, 5760, 5696,
    5632, 5568, 5504, 5440, 5376, 5312, 5248, 5184,
    5120, 5056, 4992, 4928, 4864, 4800, 4736, 4672,
    4608, 4544, 4480, 4416, 4352, 4288, 4224, 4160,
    4096, 4032, 3968, 3904, 3840, 3776, 3712, 3648,
    3584, 3520, 3456, 3392, 3328, 3264, 3200, 3136,
    3072, 3008, 2944, 2880, 2816, 2752, 2688, 2624,
    2560, 2496, 2432, 2368, 2304, 2240, 2176, 2112,
    2048, 1984, 1920, 1856, 1792, 1728, 1664, 1600,
    1536, 1472, 1408, 1344, 1280, 1216, 1152, 1088,
    1024, 960, 896, 832, 768, 704, 640, 576,
    512, 448, 384, 320, 256, 192, 128, 64,
    0, -63, -127, -191, -255, -319, -383, -447,
    -511, -575, -639, -703, -767, -831, -895, -959,
    -1023, -1087, -1151, -1215, -1279, -1343, -1407, -1471,
    -1535, -1599, -1663, -1727, -1791, -1855, -1919, -1983,
    -2047, -2111, -2175, -2239, -2303, -2367, -2431, -2495,
    -2559, -2623, -2687, -2751, -2815, -2879, -2943, -3007,
    -3071, -3135, -3199, -3263, -3327, -3391, -3455, -3519,
    -3583, -3647, -3711, -3775, -3839, -3903, -3967, -4031,
    -4095, -4159, -4223, -4287, -4351, -4415, -4479, -4543,
    -4607, -4671, -4735, -4799, -4863, -4927, -4991, -5055,
    -5119, -5183, -5247, -5311, -5375, -5439, -5503, -5567,
    -5631, -5695, -5759, -5823, -5887, -5951, -6015, -6079,
    -6143, -6207, -6271, -6335, -6399, -6463, -6527, -6591,
    -6655, -6719, -6783, -6847, -6911, -6975, -7039, -7103,
    -7167, -7231, -7295, -7359, -7423, -7487, -7551, -7615,
    -7679, -7743, -7807, -7871, -7935, -7999, -8063, -8127,
    -8191, -8255, -8319, -8383, -8447, -8511, -8575, -8639,
    -8703, -8767, -8831, -8895, -8959, -9023, -9087, -9151,
    -9215, -9279, -9343, -9407, -9471, -9535, -9599, -9663,
    -9727, -9791, -9855, -9919, -9983, -10047, -10111, -10175,
    -10239, -10303, -10367, -10431, -10495, -10559, -10623, -10687,
    -10751, -10815, -10879, -10943, -11007, -11071, -11135, -11199,
    -11263, -11327, -11391, -11455, -11519, -11583, -11647, -11711,
    -11775, -11839, -11903, -11967, -12031, -12095, -12159, -12223,
    -12287, -12351, -12415, -12479, -12543, -12607, -12671, -12735,
    -12799, -12863, -12927, -12991, -13055, -13119, -13183, -13247,
    -13311, -13375, -13439, -13503, -13567, -13631, -13695, -13759,
    -13823, -13887, -13951, -14015, -14079, -14143, -14207, -14271,
    -14335, -14399, -14463, -14527, -14591, -14655, -14719, -14783,
    -14847, -14911, -14975, -15039, -15103, -15167, -15231, -15295,
    -15359, -15423, -15487, -15551, -15615, -15679, -15743, -15807,
    -15871, -15935, -15999, -16063, -16127, -16191, -16255, -16319,
    -16383, -16447, -16511, -16575, -16639, -16703, -16767, -16831,
    -16895, -16959, -17023, -17087, -17151, -17215, -17279, -17343,
    -17407, -17471, -17535, -17599, -17663, -17727, -17791, -17855,
    -17919, -17983, -18047, -18111, -18175, -18239, -18303, -18367,
    -18431, -18495, -18559, -18623, -18687, -18751, -18815, -18879,
    -18943, -19007, -19071, -19135, -19199, -19263, -19327, -19391,
    -19455, -19519, -19583, -19647, -19711, -19775, -19839, -19903,
    -19967, -20031, -20095, -20159, -20223, -20287, -20351, -20415,
    -20479, -20543, -20607, -20671, -20735, -20799, -20863, -20927,
    -20991, -21055, -21119, -21183, -21247, -21311, -21375, -21439,
    -21503, -21567, -21631, -21695, -21759, -21823, -21887, -21951,
    -22015, -22079, -22143, -22207, -22271, -22335, -22399, -22463,
    -22527, -22591, -22655, -22719, -22783, -22847, -22911, -22975,
    -23039, -23103, -23167, -23231, -23295, -23359, -23423, -23487,
    -23551, -23615, -23679, -23743, -23807, -23871, -23935, -23999,
    -24063, -24127, -24191, -24255, -24319, -24383, -24447, -24511,
    -24575, -24639, -24703, -24767, -24831, -24895, -24959, -25023,
    -25087, -25151, -25215, -25279, -25343, -25407, -25471, -25535,
    -25599, -25663, -25727, -25791, -25855, -25919, -25983, -26047,
    -26111, -26175, -26239, -26303, -26367, -26431, -26495, -26559,
    -26623, -26687, -26751, -26815, -26879, -26943, -27007, -27071,
    -27135, -27199, -27263, -27327, -27391, -27455, -27519, -27583,
    -27647, -27711, -27775, -27839, -27903, -27967, -28031, -28095,
    -28159, -28223, -28287, -28351, -28415, -28479, -28543, -28607,
    -28671, -28735, -28799, -28863, -28927, -28991, -29055, -29119,
    -29183, -29247, -29311, -29375, -29439, -29503, -29567, -29631,
    -29695, -29759, -29823, -29887, -29951, -30015, -30079, -30143,
    -30207, -30271, -30335, -30399, -30463, -30527, -30591, -30655,
    -30719, -30783, -30847, -30911, -30975, -31039, -31103, -31167,
    -31231, -31295, -31359, -31423, -31487, -31551, -31615, -31679,
    -31743, -31807, -31871, -31935, -31999, -32063, -32127, -32191,
    -32255, -32319, -32383, -32447, -32511, -32575, -32639, -32703,
};
static const int16_t triangle_table[ks_1(KS_TABLE_BITS)] = {
    0, 127, 255, 383, 511, 639, 767, 895,
    1023, 1151, 1279, 1407, 1535, 1663, 1791, 1919,
    2047, 2175, 2303, 2431, 2559, 2687, 2815, 2943,
    3071, 3199, 3327, 3455, 3583, 3711, 3839, 3967,
    4095, 4223, 4351, 4479, 4607, 4735, 4863, 4991,
    5119, 5247, 5375, 5503, 5631, 5759, 5887, 6015,
    6143, 6271, 6399, 6527, 6655, 6783, 6911, 7039,
    7167, 7295, 7423, 7551, 7679, 7807, 7935, 8063,
    8191, 8319, 8447, 8575, 8703, 8831, 8959, 9087,
    9215, 9343, 9471, 9599, 9727, 9855, 9983, 10111,
    10239, 10367, 10495, 10623, 10751, 10879, 11007, 11135,
    11263, 11391, 11519, 11647, 11775, 11903, 12031, 12159,
    12287, 12415, 12543, 12671, 12799, 12927, 13055, 13183,
    13311, 13439, 13567, 13695, 13823, 13951, 14079, 14207,
    14335, 14463, 14591, 14719, 14847, 14975, 15103, 15231,
    15359, 15487, 15615, 15743, 15871, 15999, 16127, 16255,
    16383, 16511, 16639, 16767, 16895, 17023, 17151, 17279,
    17407, 17535, 17663, 17791, 17919, 18047, 18175, 18303,
    18431, 18559, 18687, 18815, 18943, 19071, 19199, 19327,
    19455, 19583, 19711, 19839, 19967, 20095, 20223, 20351,
    20479, 20607, 20735, 20863, 20991, 21119, 21247, 21375,
    21503, 21631, 21759, 21887, 22015, 22143, 22271, 22399,
    22527, 22655, 22783, 22911, 23039, 23167, 23295, 23423,
    23551, 23679, 23807, 23935, 24063, 24191, 24319, 24447,
    24575, 24703, 24831, 24959, 25087, 25215, 25343, 25471,
    25599, 25727, 25855, 25983, 26111, 26239, 26367, 26495,
    26623, 26751, 26879, 27007, 27135, 27263, 27391, 27519,
    27647, 27775, 27903, 28031, 28159, 28287, 28415, 28543,
    28671, 28799, 28927, 29055, 29183, 29311, 29439, 29567,
    29695, 29823, 29951, 30079, 30207, 30335, 30463, 30591,
    30719, 30847, 30975, 31103, 31231, 31359, 31487, 31615,
    31743, 31871, 31999, 32127, 32255, 32383, 32511, 32639,
    32767, 32639, 32511, 32383, 32255, 32127, 31999, 31871,
    31743, 31615, 31487, 31359, 31231, 31103, 30975, 30847,
    30719, 30591, 30463, 30335, 30207, 30079, 29951, 29823,
    29695, 29567, 29439, 29311, 29183, 29055, 28927, 28799,
    28671, 28543, 28415, 28287, 28159, 28031, 27903, 27775,
    27647, 27519, 27391, 27263, 27135, 27007, 26879, 26751,
    26623, 26495, 26367, 26239, 26111, 25983, 25855, 25727,
    25599, 25471, 25343, 25215, 25087, 24959, 24831, 24703,
    24575, 24447, 24319, 24191, 24063, 23935, 23807, 23679,
    23551, 23423, 23295, 23167, 23039, 22911, 22783, 22655,
    22527, 22399, 22271, 22143, 22015, 21887, 21759, 21631,
    21503, 21375, 21247, 21119, 20991, 20863, 20735, 20607,
    20479, 20351, 20223, 20095, 19967, 19839, 19711, 19583,
    19455, 19327, 19199, 19071, 18943, 18815, 18687, 18559,
    18431, 18303, 18175, 18047, 17919, 17791, 17663, 17535,
    17407, 17279, 17151, 17023, 16895, 16767, 16639, 16511,
    16383, 16255, 16127, 15999, 15871, 15743, 15615, 15487,
    15359, 15231, 15103, 14975, 14847, 14719, 14591, 14463,
    14335, 14207, 14079, 13951, 13823, 13695, 13567, 13439,
    13311, 13183, 13055, 12927, 12799, 12671, 12543, 12415,
    12287, 12159, 12031, 11903, 11775, 11647, 11519, 11391,
    11263, 11135, 11007, 10879, 10751, 10623, 10495, 10367,
    10239, 10111, 9983, 9855, 9727, 9599, 9471, 9343,
    9215, 9087, 8959, 8831, 8703, 8575, 8447, 8319,
    8191, 8063, 7935, 7807, 7679, 7551, 7423, 7295,
    7167, 7039, 6911, 6783, 6655, 6527, 6399, 6271,
    6143, 6015, 5887, 5759, 5631, 5503, 5375, 5247,
    5119, 4991, 4863, 4735, 4607, 4479, 4351, 4223,
    4095, 3967, 3839, 3711, 3583, 3455, 3327, 3199,
    3071, 2943, 2815, 2687, 2559, 2431, 2303, 2175,
    2047, 1919, 1791, 1663, 1535, 1407, 1279, 1151,
    1023, 895, 767, 639, 511, 383, 255, 127,
    0, -127, -255, -383, -511, -639, -767, -895,
    -1023, -1151, -1279, -1407, -1535, -1663, -1791, -1919,
    -2047, -2175, -2303, -2431, -2559, -2687, -2815, -2943,
    -3071, -3199, -3327, -3455, -3583, -3711, -3839, -3967,
    -4095, -4223, -4351, -4479, -4607, -4735, -4863, -4991,
    -5119, -5247, -5375, -5503, -5631, -5759, -5887, -6015,
    -6143, -6271, -6399, -6527, -6655, -6783, -6911, -7039,
    -7167, -7295, -7423, -7551, -7679, -7807, -7935, -8063,
    -8191, -8319, -8447, -8575, -8703, -8831, -8959, -9087,
    -9215, -9343, -9471, -9599, -9727, -9855, -9983, -10111,
    -10239, -10367, -10495, -10623, -10751, -10879, -11007, -11135,
    -11263, -11391, -11519, -11647, -11775, -11903, -12031, -12159,
    -12287, -12415, -12543, -12671, -12799, -12927, -13055, -13183,
    -13311, -13439, -13567, -13695, -13823, -13951, -14079, -14207,
    -14335, -14463, -14591, -14719, -14847, -14975, -15103, -15231,
    -15359, -15487, -15615, -15743, -15871, -15999, -16127, -16255,
    -16383, -16511, -16639, -16767, -16895, -17023, -17151, -17279,
    -17407, -17535, -17663, -17791, -17919, -18047, -18175, -18303,
    -18431, -18559, -18687, -18815, -18943, -19071, -19199, -19327,
    -19455, -19583, -19711, -19839, -19967, -20095, -20223, -20351,
    -20479, -20607, -20735, -20863, -20991, -21119, -21247, -21375,
    -21503, -21631, -21759, -21887, -22015, -22143, -22271, -22399,
    -22527, -22655, -22783, -22911, -23039, -23167, -23295, -23423,
    -23551, -23679, -23807, -23935, -24063, -24191, -24319, -24447,
    -24575, -24703, -24831, -24959, -25087, -25215, -25343, -25471,
    -25599, -25727, -25855, -25983, -26111, -26239, -26367, -26495,
    -26623, -26751, -26879, -27007, -27135, -27263, -27391, -27519,
    -27647, -27775, -27903, -28031, -28159, -28287, -28415, -28543,
    -28671, -28799, -28927, -29055, -29183, -29311, -29439, -29567,
    -29695, -29823, -29951, -30079, -30207, -30335, -30463, -30591,
    -30719, -30847, -30975, -31103, -31231, -31359, -31487, -31615,
    -31743, -31871, -31999, -32127, -32255, -32383, -32511, -32639,
    -32767, -32639, -32511, -32383, -32255, -32127, -31999, -31871,
    -31743, -31615, -31487, -31359, -31231, -31103, -30975, -30847,
    -30719, -30591, -30463, -30335, -30207, -30079, -29951, -29823,
    -29695, -29567, -29439, -29311, -29183, -29055, -28927, -28799,
    -28671, -28543, -28415, -28287, -28159, -28031, -27903, -27775,
    -27647, -27519, -27391, -27263, -27135, -27007, -26879, -26751,
    -26623, -26495, -26367, -26239, -26111, -25983, -25855, -25727,
    -25599, -25471, -25343, -25215, -25087, -24959, -24831, -24703,
    -24575, -24447, -24319, -24191, -24063, -23935, -23807, -23679,
    -23551, -23423, -23295, -23167, -23039, -22911, -22783, -22655,
    -22527, -22399, -22271, -22143, -22015, -21887, -21759, -21631,
    -21503, -21375, -21247, -21119, -20991, -20863, -20735, -20607,
    -20479, -20351, -20223, -20095, -19967, -19839, -19711, -19583,
    -19455, -19327, -19199, -19071, -18943, -18815, -18687, -18559,
    -18431, -18303, -18175, -18047, -17919, -17791, -17663, -17535,
    -17407, -17279, -17151, -17023, -16895, -16767, -16639, -16511,
    -16383, -16255, -16127, -15999, -15871, -15743, -15615, -15487,
    -15359, -15231, -15103, -14975, -14847, -14719, -14591, -14463,
    -14335, -14207, -14079, -13951, -13823, -13695, -13567, -13439,
    -13311, -13183, -13055, -12927, -12799, -12671, -12543, -12415,
    -12287, -12159, -12031, -11903, -11775, -11647, -11519, -11391,
    -11263, -11135, -11007, -10879, -10751, -10623, -10495, -10367,
    -10239, -10111, -9983, -9855, -9727, -9599, -9471, -9343,
    -9215, -9087, -8959, -8831, -8703, -8575, -8447, -8319,
    -8191, -8063, -7935, -7807, -7679, -7551, -7423, -7295,
    -7167, -7039, -6911, -6783, -6655, -6527, -6399, -6271,
    -6143, -6015, -5887, -5759, -5631, -5503, -5375, -5247,
    -5119, -4991, -4863, -4735, -4607, -4479, -4351, -4223,
    -4095, -3967, -3839, -3711, -3583, -3455, -3327, -3199,
    -3071, -2943, -2815, -2687, -2559, -2431, -2303, -2175,
    -2047, -1919, -1791, -1663, -1535, -1407, -1279, -1151,
    -1023, -895, -767, -639, -511, -383, -255, -127,
};
static const int32_t noise_table[ks_1(KS_TABLE_BITS)] = {
    27530, 12922, 25659, 26162, 29871, 6473, 10984, 25172,
    9101, 18151, 15642, 20606, 11952, 16822, 31201, 30020,
    20830, 23503, 4639, 19888, 534, 7958, 4496, 26350,
    5133, 13137, 4252, 3565, 32731, 7151, 16807, 27495,
    20074, 9700, 20890, 17179, 16173, 31874, 9584, 25275,
    17259, 25227, 13114, 29212, 9283, 11549, 26466, 30113,
    2285, 31106, 17235, 2819, 6298, 21731, 29170, 11432,
    2102, 656, 14997, 2067, 7807, 31804, 29562, 27882,
    8737, 17686, 12294, 24911, 16794, 21879, 17419, 1287,
    14340, 30533, 30499, 23623, 9315, 24199, 20970, 11601,
    22539, 5438, 14420, 28837, 27170, 10824, 7502, 29273,
    11480, 22500, 31340, 19287, 21537, 28136, 14403, 30275,
    13055, 26697, 22419, 29849, 15809, 7071, 31136, 30149,
    4838, 28869, 21006, 14153, 20302, 9209, 25754, 10074,
    14647, 7408, 6144, 9051, 18232, 13647, 5557, 29713,
    3380, 4131, 16234, 24918, 32267, 30637, 22427, 12555,
    24567, 12079, 9638, 7610, 19151, 8008, 4993, 23990,
    4111, 25999, 5377, 24413, 2442, 31132, 1721, 17090,
    5773, 7866, 26141, 24006, 21513, 31698, 20953, 24894,
    3063, 4420, 17045, 2563, 2290, 6705, 15119, 26858,
    18785, 24758, 1701, 5170, 32766, 6695, 29161, 4111,
    32694, 1771, 28524, 2369, 136, 30246, 19460, 5910,
    5345, 12834, 29917, 26858, 11766, 18103, 18986, 14829,
    22523, 3264, 17392, 24814, 9970, 32512, 18905, 28756,
    24503, 20607, 1160, 24503, 27302, 30321, 28614, 27230,
    32093, 24372, 29600, 32229, 21851, 16293, 5372, 27196,
    29128, 2522, 21288, 8127, 20626, 7508, 22957, 10382,
    10773, 7583, 2430, 20743, 7328, 21335, 16733, 31832,
    9176, 17894, 23568, 3711, 15449, 19415, 30942, 14775,
    11021, 27776, 14237, 105, 11302, 19610, 27302, 7663,
    22133, 15824, 15791, 9992, 23332, 5981, 20375, 1339,
    13565, 22805, 22082, 20893, 11373, 6049, 19958, 20550,
    23943, 10759, 24261, 6625, 30175, 22437, 21401, 8429,
    17446, 2871, 8535, 28749, 22482, 3071, 3646, 11848,
    18896, 19437, 21841, 9462, 25419, 9449, 10801, 6217,
    32254, 117, 27111, 10861, 6166, 14302, 31411, 30110,
    25062, 22906, 3969, 22471, 12577, 25370, 30900, 30023,
    28242, 6669, 26005, 17957, 9741, 29651, 29806, 28637,
    16322, 18880, 5333, 8975, 28329, 16134, 15192, 27817,
    16251, 9536, 5911, 22418, 23839, 4556, 19762, 16135,
    27463, 23731, 5839, 7273, 16335, 3973, 4529, 11810,
    10642, 30535, 29768, 20384, 27420, 26807, 16254, 10976,
    12920, 21587, 19951, 8483, 4955, 2377, 3533, 21207,
    11914, 9445, 10858, 2986, 14002, 30620, 19121, 8698,
    21585, 24961, 15971, 5153, 28934, 20501, 16963, 6810,
    18269, 13965, 27194, 12922, 8005, 10682, 23898, 20926,
    32270, 11083, 29410, 4458, 13460, 177, 25665, 25374,
    9622, 3757, 28360, 23625, 1610, 14715, 32323, 23196,
    6910, 15528, 28349, 3077, 3262, 12546, 9887, 21531,
    26511, 4315, 1687, 1750, 14997, 25586, 22677, 14501,
    3902, 19320, 18960, 17363, 19497, 11858, 9970, 29120,
    15616, 5564, 19978, 17227, 20280, 19535, 7656, 27190,
    2296, 3238, 30267, 5558, 15784, 7388, 27090, 9529,
    11704, 28778, 11280, 26702, 21598, 1190, 8436, 25501,
    20510, 27396, 10097, 7241, 6488, 20067, 3595, 22104,
    25632, 23574, 6564, 13145, 10343, 14221, 7569, 12639,
    17459, 5069, 18198, 477, 12458, 12522, 10007, 24162,
    8533, 21287, 18097, 30132, 22477, 26534, 22866, 10221,
    21163, 196, 17463, 27652, 20264, 21059, 16990, 13130,
    11866, 23555, 26275, 22209, 5009, 1077, 2082, 22469,
    6147, 20281, 22946, 18606, 36, 187, 10001, 8570,
    21474, 28099, 5936, 11185, 21866, 28802, 21406, 10263,
    28999, 6103, 5148, 16496, 27162, 22139, 29626, 6262,
    12927, 23135, 28472, 17936, 24213, 30554, 7638, 30361,
    18069, 30585, 16200, 18106, 30772, 26201, 26676, 19479,
    21534, 32612, 30665, 10634, 28648, 19304, 20897, 24880,
    25408, 26046, 8610, 19803, 15418, 5470, 26065, 28346,
    28606, 21770, 13515, 20052, 19558, 21154, 17646, 4860,
    18972, 1080, 22966, 16978, 27282, 16876, 3691, 16049,
    16722, 1589, 26683, 12604, 20894, 14814, 4717, 13535,
    8094, 13328, 572, 23513, 18799, 26637, 19092, 14638,
    15641, 32608, 1924, 2433, 20995, 19571, 7294, 7201,
    20651, 30260, 24180, 15166, 14370, 27871, 31215, 31092,
    29460, 25132, 10930, 17587, 7180, 15647, 31122, 15274,
    28976, 31694, 6021, 15008, 25565, 25114, 29646, 8440,
    24955, 31571, 10873, 13184, 18375, 18167, 20386, 6259,
    15661, 11799, 21425, 30031, 6903, 19874, 28357, 3597,
    12240, 6520, 21184, 19420, 22168, 19540, 1928, 18378,
    18468, 7950, 619, 11266, 297, 30266, 19706, 25253,
    29070, 30580, 5670, 14679, 15981, 26057, 20938, 31642,
    5089, 9597, 28907, 11993, 29471, 24497, 15590, 8944,
    31018, 4008, 28365, 20420, 23548, 30294, 6031, 9249,
    5477, 6650, 20516, 5774, 4150, 7456, 31027, 454,
    5269, 3931, 15133, 21250, 29989, 3304, 20126, 2311,
    12901, 16266, 14305, 9606, 7997, 29896, 18551, 6248,
    1137, 14150, 26669, 24686, 11677, 32700, 1168, 17155,
    6584, 21684, 22929, 10734, 29141, 21190, 11189, 1643,
    25122, 26322, 22894, 22344, 29627, 10254, 24656, 9762,
    26520, 6195, 19368, 1751, 3324, 5153, 8000, 4461,
    19303, 1902, 29147, 30981, 1835, 30316, 15369, 8420,
    19234, 5532, 19155, 15608, 26723, 30344, 17252, 19078,
    23900, 7380, 8656, 20760, 17634, 545, 30523, 11388,
    6740, 17124, 13139, 10064, 22278, 21139, 14526, 8815,
    23041, 10907, 7029, 24876, 8457, 22398, 530, 27692,
    27930, 19685, 10533, 21887, 17262, 27786, 8198, 8395,
    2399, 16854, 29156, 20034, 17400, 26912, 31422, 24140,
    11270, 11794, 1438, 781, 166, 15965, 9596, 23207,
    26873, 16626, 15317, 2564, 6257, 15847, 30256, 1421,
    2765, 8023, 23308, 20028, 3042, 31507, 28424, 5442,
    15595, 24813, 25476, 228, 18959, 24131, 24369, 30229,
    3158, 25808, 31011, 3325, 9007, 7841, 26532, 3114,
    24468, 9083, 5678, 30726, 24931, 3167, 32147, 27697,
    11191, 22689, 14958, 14233, 21430, 10615, 19676, 4258,
    2662, 12385, 4487, 21622, 3750, 28857, 19085, 6909,
    21899, 17329, 10234, 30906, 25171, 4000, 1253, 16872,
    13083, 6932, 14831, 5248, 10100, 14212, 178, 21291,
    4135, 15136, 2758, 25566, 25752, 22434, 29825, 28415,
    2053, 1546, 17270, 5804, 30403, 3588, 12713, 19535,
    20918, 22948, 17674, 13323, 26948, 18928, 30196, 7265,
    25861, 12261, 12513, 3194, 26474, 12691, 24486, 30610,
    27828, 27244, 23409, 20813, 16912, 20468, 16462, 18965,
    22014, 965, 24770, 19650, 4554, 4716, 6418, 25473,
    27664, 24093, 6029, 21845, 10255, 3459, 29111, 3349,
    15720, 8857, 6544, 9428, 21548, 31030, 7271, 16610,
    25507, 30681, 4656, 9653, 18382, 21118, 28619, 7629,
    22084, 20622, 27280, 26639, 25338, 932, 19345, 20236,
    25026, 25375, 9315, 2514, 28835, 5659, 5864, 11789,
    14517, 12409, 21217, 3299, 10672, 28488, 19909, 3413,
    26403, 24566, 13066, 12018, 12918, 8918, 19648, 2235,
    29541, 14161, 28875, 22112, 15094, 15454, 9582, 7353,
    8063, 18897, 9868, 4131, 24557, 15733, 15920, 6307,
    28142, 4370, 9606, 6048, 92, 29515, 9461, 26495,
    21314, 22528, 5747, 1465, 31447, 25396, 3701, 28221,
    6791, 32576, 17566, 21885, 15264, 27148, 29238, 23327,
    13279, 6340, 27458, 5069, 22073, 10611, 11376, 17448,
    14982, 20982, 23496, 15075, 17731, 191, 8803, 6279,
    22719, 14551, 7744, 21399, 7181, 11446, 16853, 13972,
    11256, 1653, 3090, 26520, 28802, 32329, 17080, 9315,
};
static const uint32_t note_freq[128] = {
    535809, 567670, 601425, 637188, 675077, 715219, 757748, 802806,
    850544, 901120, 954703, 1011473, 1071618, 1135340, 1202850, 1274376,
    1350154, 1430438, 1515497, 1605613, 1701088, 1802240, 1909406, 2022946,
    2143236, 2270680, 2405701, 2548752, 2700308, 2860877, 3030994, 3211226,
    3402176, 3604480, 3818813, 4045892, 4286473, 4541360, 4811403, 5097504,
    5400617, 5721755, 6061988, 6422453, 6804352, 7208960, 7637627, 8091784,
    8572946, 9082720, 9622807, 10195009, 10801235, 11443510, 12123977, 12844906,
    13608704, 14417920, 15275254, 16183568, 17145893, 18165440, 19245614, 20390018,
    21602471, 22887021, 24247954, 25689812, 27217408, 28835840, 30550508, 32367136,
    34291786, 36330881, 38491228, 40780036, 43204943, 45774042, 48495908, 51379625,
    54434817, 57671680, 61101016, 64734272, 68583572, 72661763, 76982456, 81560072,
    86409886, 91548085, 96991817, 102759251, 108869634, 115343360, 122202033, 129468544,
    137167144, 145323527, 153964913, 163120144, 172819772, 183096171, 193983635, 205518503,
    217739269, 230686720, 244404066, 258937088, 274334288, 290647054, 307929827, 326240288,
    345639545, 366192342, 387967271, 411037006, 435478538, 461373440, 488808132, 517874176,
    548668577, 581294108, 615859655, 652480576, 691279090, 732384684, 775934543, 822074012,
};
static const int32_t ratescale[128] = {
    3461439, 3263485, 3076641, 2900284, 2733826, 2576709, 2428412, 2288437,
    2156319, 2031616, 1913911, 1802814, 1697951, 1598974, 1505552, 1417374,
    1334145, 1255586, 1181438, 1111450, 1045391, 983040, 924187, 868639,
    816207, 766719, 720008, 675919, 634304, 595025, 557951, 522957,
    489927, 458752, 429325, 401551, 375335, 350591, 327236, 305191,
    284384, 264744, 246207, 228710, 212195, 196608, 181894, 168007,
    154899, 142527, 130850, 119827, 109424, 99604, 90335, 81587,
    73329, 65536, 58179, 51235, 44681, 38495, 32657, 27145,
    21944, 17034, 12399, 8025, 3896, 0, -3679, -7151,
    -10428, -13521, -16440, -19196, -21796, -24251, -26569, -28756,
    -30820, -32768, -34608, -36344, -37982, -39529, -40988, -42366,
    -43666, -44894, -46053, -47146, -48178, -49152, -50072, -50940,
    -51759, -52533, -53262, -53951, -54601, -55215, -55795, -56341,
    -56857, -57344, -57804, -58238, -58648, -59035, -59399, -59744,
    -60069, -60376, -60666, -60939, -61197, -61440, -61670, -61887,
    -62092, -62286, -62468, -62640, -62803, -62956, -63101, -63238,
};
static const uint16_t keyscale_curves[4][128] = {
    {
        65535, 61856, 58385, 55108, 52015, 49095, 46340, 43739,
        41284, 38967, 36780, 34715, 32767, 30928, 29192, 27554,
        26007, 24547, 23170, 21869, 20642, 19483, 18390, 17357,
        16383, 15464, 14596, 13777, 13003, 12273, 11585, 10934,
        10321, 9741, 9195, 8678, 8191, 7732, 7298, 6888,
        6501, 6136, 5792, 5467, 5160, 4870, 4597, 4339,
        4095, 3866, 3649, 3444, 3250, 3068, 2896, 2733,
        2580, 2435, 2298, 2169, 2047, 1933, 1824, 1722,
        1625, 1534, 1448, 1366, 1290, 1217, 1149, 1084,
        1023, 966, 912, 861, 812, 767, 724, 683,
        645, 608, 574, 542, 511, 483, 456, 430,
        406, 383, 362, 341, 322, 304, 287, 271,
        255, 241, 228, 215, 203, 191, 181, 170,
        161, 152, 143, 135, 127, 120, 114, 107,
        101, 95, 90, 85, 80, 76, 71, 67,
        63, 60, 57, 53, 50, 47, 45, 42,
        },
    {
        65408, 64897, 64386, 63875, 63364, 62853, 62342, 61831,
        61320, 60809, 60298, 59787, 59276, 58765, 58254, 57743,
        57232, 56721, 56210, 55699, 55188, 54677, 54166, 53655,
        53144, 52633, 52122, 51611, 51100, 50589, 50078, 49567,
        49056, 48545, 48034, 47523, 47012, 46501, 45990, 45479,
        44968, 44457, 43946, 43435, 42924, 42413, 41902, 41391,
        40880, 40369, 39858, 39347, 38836, 38325, 37814, 37303,
        36792, 36281, 35770, 35259, 34748, 34237, 33726, 33215,
        32704, 32193, 31682, 31171, 30660, 30149, 29638, 29127,
        28616, 28105, 27594, 27083, 26572, 26061, 25550, 25039,
        24528, 24017, 23506, 22995, 22484, 21973, 21462, 20951,
        20440, 19929, 19418, 18907, 18396, 17885, 17374, 16863,
        16352, 15841, 15330, 14819, 14308, 13797, 13286, 12775,
        12264, 11753, 11242, 10731, 10220, 9709, 9198, 8687,
        8176, 7665, 7154, 6643, 6132, 5621, 5110, 4599,
        4088, 3577, 3066, 2555, 2044, 1533, 1022, 511,
        },
    {
        0, 3679, 7150, 10427, 13520, 16440, 19195, 21796,
        24251, 26568, 28755, 30820, 32768, 34607, 36343, 37981,
        39528, 40988, 42365, 43666, 44893, 46052, 47145, 48178,
        49152, 50071, 50939, 51758, 52532, 53262, 53950, 54601,
        55214, 55794, 56340, 56857, 57344, 57803, 58237, 58647,
        59034, 59399, 59743, 60068, 60375, 60665, 60938, 61196,
        61440, 61669, 61886, 62091, 62285, 62467, 62639, 62802,
        62955, 63100, 63237, 63366, 63488, 63602, 63711, 63813,
        63910, 64001, 64087, 64169, 64245, 64318, 64386, 64451,
        64512, 64569, 64623, 64674, 64723, 64768, 64811, 64852,
        64890, 64927, 64961, 64993, 65024, 65052, 65079, 65105,
        65129, 65152, 65173, 65194, 65213, 65231, 65248, 65264,
        65280, 65294, 65307, 65320, 65332, 65344, 65354, 65365,
        65374, 65383, 65392, 65400, 65408, 65415, 65421, 65428,
        65434, 65440, 65445, 65450, 65455, 65459, 65464, 65468,
        65472, 65475, 65478, 65482, 65485, 65488, 65490, 65493,
        },
    {
        511, 1022, 1533, 2044, 2555, 3066, 3577, 4088,
        4599, 5110, 5621, 6132, 6643, 7154, 7665, 8176,
        8687, 9198, 9709, 10220, 10731, 11242, 11753, 12264,
        12775, 13286, 13797, 14308, 14819, 15330, 15841, 16352,
        16863, 17374, 17885, 18396, 18907, 19418, 19929, 20440,
        20951, 21462, 21973, 22484, 22995, 23506, 24017, 24528,
        25039, 25550, 26061, 26572, 27083, 27594, 28105, 28616,
        29127, 29638, 30149, 30660, 31171, 31682, 32193, 32704,
        33215, 33726, 34237, 34748, 35259, 35770, 36281, 36792,
        37303, 37814, 38325, 38836, 39347, 39858, 40369, 40880,
        41391, 41902, 42413, 42924, 43435, 43946, 44457, 44968,
        45479, 45990, 46501, 47012, 47523, 48034, 48545, 49056,
        49567, 50078, 50589, 51100, 51611, 52122, 52633, 53144,
        53655, 54166, 54677, 55188, 55699, 56210, 56721, 57232,
        57743, 58254, 58765, 59276, 59787, 60298, 60809, 61320,
        61831, 62342, 62853, 63364, 63875, 64386, 64897, 65408,
        },
};



// liniar interpolution
inline int16_t ks_table_value_li(const int16_t* table, uint32_t phase, unsigned mask)
{
    unsigned index_m = phase >> KS_PHASE_BITS;
    unsigned index_b = (index_m + 1);

    uint32_t under_fixed_b = ks_mask(phase, KS_PHASE_BITS);
    uint32_t under_fixed_m = ks_1(KS_PHASE_BITS) - under_fixed_b;

    int64_t sin_31 = table[index_m & mask] * under_fixed_m +
        table[index_b & mask] * under_fixed_b;
     sin_31 >>= KS_PHASE_BITS;

    return (int16_t)sin_31;
}

// sin table value
inline int16_t ks_sin(uint32_t phase, bool linear_interpolution)
{
    return linear_interpolution ? ks_table_value_li(sin_table, phase,  ks_m(KS_TABLE_BITS)) :
                                  sin_table[ks_mask(phase >> KS_PHASE_BITS, KS_TABLE_BITS)];
}

inline int16_t ks_saw(uint32_t phase)
{
    return saw_table[ks_mask((phase >> KS_PHASE_BITS), KS_TABLE_BITS)];
}

inline int16_t ks_triangle(uint32_t phase)
{
    return triangle_table[ks_mask((phase >> KS_PHASE_BITS), KS_TABLE_BITS)];
}

inline int16_t ks_fake_triangle(uint32_t phase, uint32_t shift)
{
    return triangle_table[ks_mask((phase >> (KS_PHASE_BITS +shift) << (shift)), KS_TABLE_BITS)];
}

inline int16_t ks_noise(uint32_t phase, uint32_t begin){
    return noise_table[ks_mask((begin + ((phase >> KS_NOISE_PHASE_BITS))), KS_TABLE_BITS)];
}

inline uint32_t ks_notefreq(uint8_t notenumber){
    return note_freq[notenumber];
}

inline int32_t ks_ratescale(uint8_t index){
    return ratescale[index];
}

inline uint16_t ks_keyscale_curves(uint8_t type, uint8_t index){
    return keyscale_curves[type][index];
}
