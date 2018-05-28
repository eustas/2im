goog.require('goog.testing.asserts');
goog.require('goog.testing.jsunit');

function testGolden() {
  let triplets = [
    8002, 29338, 61815, 11202, 13541, 15183, 7458, 11303, 38428, 29856, 31071, 31915, 1589, 32595, 53464, 7672, 8537,
    9070, 26967, 46483, 63228, 5223, 8472, 14582, 23664, 26995, 27935, 37769, 41292, 48104, 1314, 7395, 8494, 23926,
    42759, 53899, 24622, 37781, 57985, 17762, 24766, 26108, 8511, 15545, 30948, 11498, 25088, 26836, 8917, 12497,
    24655, 11910, 12012, 12986, 9694, 48084, 57951, 4175, 6657, 14895, 2459, 3017, 6814, 7721, 13721, 22763, 7637,
    9303, 14448, 14855, 17378, 37428, 11918, 15000, 15810, 18809, 21358, 22395, 31543, 32536, 33515, 9944, 10590,
    12142, 39684, 41153, 55275, 37716, 39162, 39453, 7149, 15547, 19626, 956, 9743, 15733, 29126, 37518, 48428, 2384,
    5898, 9034, 27261, 28291, 35406, 6126, 8000, 11348, 18139, 20140, 25268, 5468, 6632, 9541, 19381, 34441, 39329,
    25152, 29988, 43713, 44250, 47213, 57368, 1365, 1393, 1407, 48565, 50403, 50460, 12150, 18569, 46614, 7663, 15567,
    15971, 188, 16885, 57629, 3003, 24822, 35506, 40311, 53877, 64986, 1317, 1344, 1470, 24186, 41070, 55429, 11727,
    12699, 26544, 18594, 53160, 63652, 11238, 15267, 22743, 5634, 33892, 41195, 42409, 46591, 47352, 487, 1545, 49653,
    25459, 41513, 41526, 15160, 39144, 49262, 23, 3395, 3498, 9646, 30752, 48233, 20774, 46576, 57474, 12643, 23821,
    31047, 7685, 11091, 11912, 9949, 31036, 40301, 19584, 27227, 29881, 614, 30572, 32083, 28635, 32194, 50294,
    10911, 27030, 57355, 1326, 6873, 11690, 12753, 18002, 45894, 7757, 32782, 32839, 846, 847, 5952, 19095, 21758,
    22506, 23357, 29047, 29632, 1026, 21746, 37991, 8773, 13139, 30284, 3415, 7102, 18014, 4884, 29513, 60781, 6746,
    8560, 14733, 23883, 24254, 29309, 20240, 35640, 65088, 6796, 24639, 48566, 7995, 12253, 30994, 11675, 12120,
    13905, 5610, 5724, 6107, 2380, 3547, 5651, 3008, 3149, 3560, 53735, 55576, 56885, 2498, 6449, 7188, 17908, 61790,
    65291, 1709, 6600, 28410, 464, 1360, 4857, 17134, 18704, 35718, 29095, 38791, 57114, 33541, 34773, 63127, 4419,
    19925, 35465, 31428, 40876, 42992, 12153, 28551, 30756, 5261, 9157, 10685, 6563, 11639, 35850, 980, 6296, 8370,
    1984, 3815, 5920, 7351, 11563, 51878, 8356, 8662, 40412, 11260, 37217, 63771, 2934, 32531, 59994, 8464, 9171,
    11076, 4896, 13775, 41555, 37560, 40135, 45011, 47976, 50350, 63642, 394, 445, 456, 5193, 29863, 34648, 5846,
    12992, 41856, 14809, 20875, 20875, 7050, 13953, 20920, 10818, 17070, 17941, 7789, 15091, 28387, 33471, 41148,
    64365, 2577, 3171, 11075, 34111, 39392, 50755, 7782, 20127, 65394, 4552, 10386, 16028, 20609, 33447, 34611, 29421,
    35610, 40264, 5115, 8890, 14853, 88, 1093, 2551, 3218, 3996, 13211, 60510, 60600, 61387, 15674, 18057, 21225,
    13909, 24625, 48884, 997, 2370, 2933, 8615, 25017, 40345, 14760, 21829, 34493, 1982, 4678, 9905, 34609, 39503,
    56518, 16987, 32527, 34647, 28346, 35254, 37169, 7594, 9980, 13627, 39504, 42030, 54543, 7941, 8599, 11311, 12422,
    14981, 17215, 2124, 32541, 37831, 7495, 16346, 52771, 5078, 8018, 8056, 28514, 29265, 40241, 2447, 14480, 22479,
    26573, 29358, 50374, 881, 11788, 17294, 13266, 13299, 13998, 2698, 19999, 27209, 1906, 5305, 23152, 4541, 6820,
    13316, 9401, 14961, 59844, 44855, 45362, 45527, 3283, 36478, 56119, 2051, 33552, 34354, 5303, 6588, 16977, 9494,
    15693, 17135, 34955, 34996, 35711, 301, 15215, 16394, 1132, 2791, 15040, 18666, 26633, 33752, 16009, 16578, 19946,
    1989, 4357, 8890, 1829, 1975, 10003, 1918, 2337, 7805, 6468, 9486, 11370, 14561, 36501, 48272, 2920, 5644, 10414,
    19385, 23573, 47398, 15178, 15602, 15958, 561, 766, 1199, 5523, 6089, 11733, 7638, 7891, 8255, 10979, 26605,
    27775, 57045, 62958, 63327, 44840, 45361, 57420, 41559, 52556, 55217, 44031, 56901, 62113, 63737, 63774, 64326,
    11386, 11681, 17508, 13983, 19531, 59705, 3034, 4965, 20009, 1414, 2070, 2880, 22983, 23748, 26203, 15594, 22033,
    22516, 2841, 40536, 50437, 11458, 32845, 50606, 31629, 32270, 52260, 24458, 27423, 30456, 6257, 15736, 26244,
    42681, 46216, 46452, 22258, 27112, 28503, 1078, 16636, 45840, 27305, 31552, 35882, 8994, 16110, 53959, 5869,
    30505, 31410, 1451, 8785, 21793, 4864, 26513, 31614, 8398, 12982, 20038, 12262, 36708, 39439, 35750, 44256, 44364,
    885, 958, 1001, 27794, 44641, 63565, 14825, 16804, 18460, 8552, 25180, 46384, 6633, 19767, 20364, 9432, 22485,
    52490, 20220, 24095, 37705, 43368, 43662, 62361, 11774, 25306, 47929, 26803, 27009, 29653, 23538, 25604, 31090,
    34465, 35386, 39942, 15155, 25107, 25448, 13782, 22631, 31204, 18353, 19545, 22497, 1263, 2203, 2753, 3957, 13534,
    16706, 3576, 7813, 11829, 3674, 5492, 25019, 7343, 20989, 51492, 7607, 12228, 42867, 48669, 49436, 49689, 8181,
    14678, 29201, 43112, 46723, 53087, 28756, 37788, 47324, 20401, 25314, 30751, 15447, 30097, 30378, 11975, 15915,
    18380, 42917, 48170, 65416, 12280, 36174, 36324, 37653, 40712, 44493, 13674, 22215, 24030, 4873, 10424, 40682,
    7433, 13670, 19735, 4924, 10601, 53647, 34142, 47661, 48735, 2708, 7403, 7537, 25157, 27002, 30972, 9399, 18351,
    22373, 2691, 7064, 10751, 24654, 25707, 27995, 24804, 31070, 38921, 7393, 8640, 10340, 33979, 35035, 36003, 7671,
    13621, 36696, 19393, 22153, 26951, 18267, 20856, 42714, 9542, 10625, 21943, 36199, 52306, 55757, 55449, 63609,
    64293, 38856, 44196, 46583, 26583, 36905, 40406, 20437, 27965, 61634, 2360, 12646, 12653, 30409, 32348, 33168,
    38231, 41586, 63334, 10697, 20187, 48135, 5034, 14767, 15048, 19669, 25213, 27519, 3522, 20106, 32693, 7939,
    61351, 62992, 2870, 13859, 14146, 13529, 14577, 15189, 2331, 61148, 63396, 4935, 6369, 11733, 30423, 35089, 41365,
    5181, 7610, 8506, 36721, 48769, 54235, 31450, 31835, 35716, 29080, 29156, 30781, 28109, 29428, 30425, 11210,
    28003, 32573, 8037, 11060, 13052, 29316, 29323, 33290, 14267, 17708, 20171, 20641, 29113, 36135, 19069, 28218,
    31295, 33765, 40654, 49268, 28953, 38513, 46366, 15856, 34009, 38421, 5841, 7598, 11590, 7439, 19741, 24617, 6685,
    6719, 8053, 17394, 19604, 40698, 15639, 20687, 63180, 7983, 28024, 31441, 32317, 33711, 35445, 8660, 26801, 37875,
    20944, 31608, 36320, 12440, 19429, 19479, 21717, 30919, 31106, 19369, 34262, 53390, 4382, 7233, 9750, 10084,
    10130, 11118, 19843, 37968, 53002, 4373, 12082, 23701, 10360, 36763, 39571, 7474, 32334, 36017, 19752, 37882,
    52055, 1671, 9620, 19128, 522, 801, 1773, 5957, 25742, 26469, 1222, 29259, 30747, 1585, 3312, 4206, 5488, 43948,
    47752, 8729, 23116, 59265, 2745, 8265, 9234, 12622, 12678, 12990, 5699, 10668, 18286, 49302, 61307, 63342, 3736,
    4810, 5722, 50462, 54250, 58351, 7644, 36416, 37794, 38503, 38514, 39327, 11821, 18050, 18613, 7389, 10691, 14647,
    19839, 21724, 24310, 5570, 36919, 50452, 10859, 14742, 15643, 1317, 6226, 12125, 6542, 13235, 20126, 5567, 13420,
    29523, 1607, 3531, 6708, 8656, 24142, 28671, 50622, 52510, 52858, 2697, 6454, 15472, 9336, 12014, 21827, 28237,
    30003, 33133, 12503, 26344, 40328, 29431, 32356, 32406, 21782, 36072, 48111, 5240, 47146, 65216, 5689, 31898,
    43613, 48114, 53108, 61088, 19328, 27775, 38428, 36251, 36620, 36664, 15031, 35421, 40241, 8893, 9390, 9921, 9016,
    24515, 32099, 15408, 23565, 27046, 32179, 35242, 35376, 17314, 25220, 31604, 137, 1541, 3470, 14820, 17400, 17483,
    5065, 23134, 24502, 27977, 63989, 65022, 35739, 35856, 38190, 2907, 3512, 4650, 24738, 27212, 44006, 127, 3501,
    6913, 33549, 48516, 51289, 1825, 18305, 62933, 18795, 23402, 56129, 30509, 48572, 64352, 1295, 6011, 16031, 10584,
    11302, 26134, 6994, 31688, 33131, 22282, 30800, 35346, 14615, 17803, 19016, 34778, 46127, 54336, 3849, 4367, 7581,
    23526, 25105, 25247, 32015, 36260, 50803, 28255, 42378, 54573, 44, 142, 183, 40059, 40201, 40629, 32662, 39104,
    40326, 5543, 6938, 9901, 468, 571, 571, 13650, 23757, 29926, 1654, 27019, 34435, 1273, 4689, 8734, 29748, 32490,
    59318, 6671, 15054, 25675, 17762, 19829, 20361, 34054, 34184, 42034, 8552, 15773, 15812, 12244, 35294, 36924,
    16620, 44324, 47989, 37848, 56775, 63383, 43728, 45526, 46760, 1354, 16829, 20700, 27533, 27705, 47419, 20167,
    21212, 24255, 16162, 16608, 24421, 17362, 18232, 33121, 7534, 17974, 33463, 37381, 47425, 54977, 9206, 14606,
    27862, 57376, 61142, 61348, 10420, 45380, 61263, 3907, 10558, 11280, 13705, 22931, 27986, 12054, 13323, 33334,
    2774, 7009, 10846, 26648, 27340, 30815, 1788, 2771, 5310, 8700, 12738, 16139, 28839, 36105, 44794, 6634, 17178,
    18714, 28034, 50635, 65354, 30227, 42355, 44289, 14885, 20141, 20397, 32979, 37243, 38495, 45621, 46985, 58885,
    60560, 62291, 64128, 27999, 31053, 35434, 839, 6856, 7369, 11947, 11998, 12045, 22676, 32246, 37681, 258, 4416,
    5204, 9964, 34130, 51109, 7009, 9704, 13539, 2594, 3208, 6895, 26414, 28256, 32708, 20975, 30604, 32520, 20128,
    24888, 25613, 4331, 5373, 9310, 17290, 59551, 62905, 33747, 37186, 42052, 5047, 25393, 36823, 234, 396, 706, 144,
    372, 1150, 19536, 30862, 49514, 7212, 44817, 58572, 5818, 9805, 10428, 16052, 34686, 36053, 46048, 48986, 52366,
    19369, 19588, 19911, 1361, 2147, 3246, 4356, 4750, 26188, 44184, 55401, 59104, 11272, 13102, 13749, 8717, 10881,
    16446, 33740, 43376, 45173, 39643, 40706, 43413, 6750, 16946, 26598, 17672, 20768, 36529, 5164, 11615, 12826, 35,
    74, 100, 21321, 23752, 41564, 2271, 19112, 40450, 815, 1448, 7917, 21203, 43471, 53772, 33794, 42098, 52227,
    39062, 50446, 54541, 812, 3617, 6316, 27328, 32654, 33915, 11980, 49293, 56178, 26400, 35639, 38380, 61378, 61417,
    63365, 48176, 51549, 60076, 7592, 18040, 23839, 6971, 7631, 8990, 5553, 15668, 34786, 3471, 25148, 45041, 14650,
    20562, 22750, 6774, 10534, 34932, 3669, 9449, 59197, 3751, 16808, 40323, 1203, 19701, 39266, 22777, 27114, 27225,
    16164, 27143, 59077, 6077, 25015, 36425, 9091, 10410, 11468, 4286, 49572, 50949, 7180, 20466, 65442, 26364, 49403,
    57295, 9986, 10473, 21970, 3618, 5273, 8658, 2911, 3601, 4558, 5994, 9176, 12448, 8707, 9786, 10643, 37956, 43469,
    45445, 31913, 42943, 47572, 5866, 11383, 12884, 9064, 11229, 12650, 25966, 26170, 31511, 16310, 21502, 23579,
    13057, 18737, 55829, 3380, 23029, 43473, 10889, 19637, 35795, 15560, 38174, 43777, 45666, 47881, 54346, 4910,
    5868, 19051, 6652, 6851, 6976, 52023, 59460, 63862, 17641, 32115, 33023, 56730, 57053, 61214, 18314, 20896, 24671,
    43200, 43819, 60129, 928, 1040, 1358, 16834, 17226, 25101, 28652, 29127, 33143, 8999, 18299, 19549, 19920, 21403,
    62559, 49356, 57327, 59692, 2562, 37637, 45861, 2249, 2769, 8205, 2528, 2844, 28408, 13394, 35134, 62435, 9703,
    10205, 24179, 6233, 6948, 8833, 6020, 6513, 15732, 7019, 12161, 19517, 34887, 44739, 52900, 6480, 17640, 37170,
    4854, 37303, 41731, 12543, 15280, 55194, 10924, 17613, 21127, 7260, 20595, 29033, 5892, 24451, 30049, 2220, 7018,
    11066, 202, 2163, 4786, 23529, 29014, 32115, 10123, 14240, 18211, 35479, 39862, 52458, 18617, 37863, 53839, 9603,
    30101, 42008, 8007, 8744, 16177, 27464, 28959, 52974, 25659, 32085, 58156, 20255, 21316, 22925, 2741, 3004, 3068,
    15622, 40283, 53980, 16670, 17514, 21550, 278, 319, 507, 8521, 21887, 26303, 7901, 10647, 16652, 27766, 28400,
    41720, 11859, 17505, 25217, 13531, 32081, 59600, 10046, 21550, 25789, 58, 18068, 25349, 9559, 9642, 9848, 17916,
    21162, 52557, 38275, 42580, 63254, 11059, 16205, 36298, 14964, 16404, 16860, 18020, 18101, 18905, 30430, 40426,
    43112, 25976, 48439, 50413, 35722, 45333, 59356, 10049, 24937, 52424, 926, 11152, 13918, 10433, 14894, 16806,
    11729, 15360, 17120, 24233, 42436, 60267, 11199, 12986, 17075, 11657, 25878, 36569, 1481, 18083, 27534, 26329,
    35212, 37637, 7597, 9593, 12172, 14532, 15057, 24343, 46615, 51374, 54058, 75, 76, 76, 48501, 57518, 59436, 22900,
    33996, 49247, 28707, 30690, 31924, 27877, 41018, 42611, 9221, 11716, 28039, 50592, 52172, 54663, 39679, 47724,
    48279, 20234, 56380, 64988, 54598, 55046, 55680, 3962, 32965, 56609, 823, 16280, 35255, 15067, 29446, 52766, 1088,
    1772, 2209, 4534, 12843, 14144, 10241, 41508, 45432, 42261, 53929, 64770, 12166, 16081, 19493, 33563, 35728,
    60287, 6274, 28687, 62565, 9845, 26991, 63210, 1748, 7176, 7256, 202, 21157, 52977, 8635, 20473, 26476, 412, 2791,
    2808, 14215, 17941, 18731, 2419, 10689, 19092, 7101, 9492, 40832, 22777, 23627, 26731, 15124, 17013, 17182, 1258,
    9366, 9939, 28506, 35559, 39009, 711, 1190, 2405, 4354, 40112, 52560, 34705, 36482, 62387, 41625, 47390, 56325,
    28634, 40603, 43976, 4604, 6900, 22768, 13638, 14062, 16311, 21845, 35668, 54419, 46857, 47304, 55648, 27823,
    39725, 61109, 40975, 53128, 62927, 488, 917, 1930, 2504, 35951, 61306, 20114, 24405, 31407, 19138, 55365, 56474,
    5026, 6220, 7063, 22773, 23145, 33102, 23177, 23480, 32207, 12546, 18307, 19352, 12881, 15539, 21600, 21473,
    28300, 29426, 3992, 6859, 9604, 1389, 1390, 1440, 30717, 42390, 46517, 29260, 32340, 51642, 79, 85, 120, 35055,
    35747, 38698, 11374, 13031, 21367, 26205, 33270, 40287, 1400, 13801, 32453, 4330, 15155, 15215, 11620, 46197,
    49714, 20308, 21737, 29070, 104, 270, 1853, 8328, 35960, 53402, 48319, 58647, 61208, 45554, 52379, 54046, 9477,
    10535, 10780, 19260, 32805, 49764, 13161, 20232, 25158, 7388, 15226, 38484, 11368, 39031, 46651, 13582, 19892,
    36554, 5768, 13106, 38136, 40624, 41370, 58793, 6847, 27496, 43247, 27052, 56210, 62235, 1601, 18841, 21459,
    15803, 16656, 16742, 34502, 44927, 60478, 7818, 10952, 17474, 6290, 6748, 21840, 3913, 5121, 6640, 118, 1297,
    2884, 11442, 28874, 39981, 11491, 12486, 12852, 88, 1817, 14126, 14474, 30326, 48113, 25818, 26252, 37882, 43941,
    44109, 48092, 668, 2201, 14714, 7450, 14973, 49059, 3716, 12847, 14137, 26912, 27235, 28314, 1440, 7285, 10720,
    36882, 50109, 60475, 40945, 49363, 50263, 774, 1305, 7928, 51446, 53014, 62151, 25964, 26618, 36016, 292, 388,
    403, 19439, 23593, 25831, 17166, 17693, 19803, 27725, 42872, 60983, 19870, 20304, 20679, 1271, 2537, 4169, 10340,
    23460, 62909, 2980, 4894, 13438, 47030, 48520, 61210, 28424, 29259, 48369, 51828, 53006, 65029, 9215, 10296,
    10325, 7168, 32320, 44216, 18114, 24050, 33182, 23468, 28522, 47401, 42611, 57268, 61141, 37892, 48771, 60458,
    34230, 35572, 35735, 21449, 30148, 47320, 9647, 10294, 40835, 10021, 21760, 29920, 2044, 3878, 4085, 29120, 38380,
    44684, 27189, 41213, 52467, 19646, 63575, 63915, 21623, 23427, 25130, 745, 844, 1793, 30470, 37865, 59535, 13192,
    26035, 36354, 34380, 36213, 48602, 49948, 53198, 55764, 2736, 7481, 13939, 16438, 16938, 23693, 10203, 10220,
    44533, 3824, 7503, 15286, 20949, 28719, 58625, 1664, 1917, 2746, 2821, 8152, 13697, 6033, 18196, 19114, 855, 980,
    1139, 18963, 20450, 24679, 13966, 23531, 29943, 44475, 45814, 63271, 2036, 2470, 8233, 4539, 7340, 9201, 5178,
    14527, 29269, 6096, 37913, 54642, 6118, 15119, 19737, 592, 2048, 2791, 2844, 3454, 40996, 7161, 11119, 19894,
    32224, 40496, 43562, 301, 1444, 6651, 20099, 22445, 28899, 35204, 37258, 41521, 39908, 41869, 46860, 36229, 36819,
    37960, 9176, 35501, 40618, 7078, 8184, 14039, 1925, 2067, 4141, 18188, 25026, 30352, 1639, 3204, 15187, 34274,
    36702, 47090, 1425, 8561, 10647, 7304, 27247, 34574, 2451, 13610, 16658, 13342, 24017, 33598, 7526, 34190, 35344,
    30820, 42732, 57888, 10175, 21366, 22494, 35177, 35612, 37721, 31382, 46864, 58223, 140, 1824, 4022, 8666, 11772,
    13907, 726, 3469, 3656, 38187, 41491, 42807, 36297, 37016, 37154, 13069, 17039, 17892, 1733, 5425, 6020, 22847,
    23038, 28518, 9327, 11561, 17172, 36258, 38231, 38932, 163, 7197, 10936, 31790, 31840, 44376, 3103, 7081, 14451,
    1478, 12335, 21919, 21950, 48390, 49255, 37119, 37306, 37753, 3508, 46830, 63625, 19081, 21172, 28338, 882, 6575,
    28477, 7922, 27812, 32101, 1690, 21155, 31192, 46716, 47411, 48537, 28408, 36543, 60988, 5752, 12787, 13320,
    11433, 14635, 17253, 598, 1400, 2277, 6, 637, 855, 1162, 2913, 11244, 14422, 41575, 54333, 23356, 24568, 24662,
    3017, 22661, 27073, 10025, 11677, 14887, 33875, 39607, 40191, 15626, 26430, 28149, 5851, 22379, 24979, 28226,
    30089, 35723, 8929, 19790, 21830, 13961, 14567, 15321, 56071, 56727, 57683, 39682, 48217, 55635, 16000, 19382,
    28520, 3699, 4275, 5359, 11965, 12118, 12341, 15242, 33321, 45411, 29412, 34701, 37951, 25569, 51720, 64507, 3374,
    5785, 5923, 1193, 3245, 5632, 620, 51946, 59074, 3106, 17086, 26191, 9162, 10274, 21948, 57393, 61307, 64654,
    1879, 54973, 57498, 23744, 36268, 40897, 29458, 41908, 51475, 2424, 3973, 11197, 17267, 29731, 48541, 15785,
    16018, 24818, 30022, 34280, 51942, 8672, 10929, 25368, 1784, 1950, 2156, 14665, 19836, 37141, 644, 749, 764,
    23683, 24968, 26358, 4110, 4365, 7912, 601, 1178, 1641, 27740, 46598, 57370, 33342, 45031, 47094, 25240, 37867,
    44450, 56105, 60796, 62879, 10160, 31787, 35938, 6833, 9036, 13194, 19151, 27179, 46667, 930, 55882, 62961, 45641,
    46759, 47199, 1614, 5536, 18167, 30674, 36026, 51947, 5186, 21469, 24223, 1257, 18209, 31464, 2643, 12180, 34484,
    2819, 18160, 40452, 8526, 15638, 55450, 31284, 37370, 41023, 31929, 32788, 33976, 46291, 48503, 52100, 2984,
    11758, 21757, 20989, 28891, 41716, 7691, 26106, 26627, 1514, 1878, 1955, 12801, 32130, 62591, 35923, 37197, 41803,
    43379, 43479, 43522, 26062, 28178, 29532, 5150, 10312, 36348, 7858, 35039, 43099, 6257, 10007, 12905, 13058,
    14383, 17193, 24052, 24817, 26622, 1822, 20397, 22288, 11647, 39317, 58162, 26299, 42592, 51022, 55837, 56153,
    56382, 21088, 36021, 53532, 9877, 12397, 13200, 1550, 6738, 9846, 4391, 9428, 12638, 7001, 9084, 16061, 8477,
    9413, 17794, 2840, 7202, 12126, 9884, 10894, 12291, 27393, 29551, 29790, 7310, 37763, 38145, 9578, 13934, 52486,
    9116, 14913, 16986, 5575, 9312, 28363, 21313, 53241, 64856, 1695, 5824, 12088, 7612, 10620, 12504, 36582, 54271,
    61514, 34094, 52706, 63438, 10440, 11349, 13928, 27727, 29202, 29292, 1818, 13372, 13393, 6259, 8611, 8691, 1418,
    3186, 19292, 1012, 1038, 10781, 20895, 29047, 31104, 56359, 56635, 57552, 1599, 20732, 28659, 3786, 9447, 11739,
    3168, 3840, 30711, 2099, 10150, 19481, 243, 948, 952, 4786, 8882, 11030, 79, 44849, 55077, 127, 138, 152, 14375,
    26663, 46240, 2503, 2769, 17130, 7476, 22071, 24735, 10038, 11611, 30369, 9369, 9607, 13983, 14800, 20897, 23155,
    6266, 13577, 14052, 10037, 20946, 34745, 15247, 16345, 17421, 7024, 8604, 10657, 22133, 23051, 24112, 21287,
    50693, 53494, 30464, 49304, 49490, 32406, 33846, 38008, 34835, 40989, 46156, 318, 2868, 5379, 20692, 25753, 40973,
    31229, 32545, 38420, 11421, 16220, 16680, 471, 1254, 1272, 26825, 40215, 52638, 978, 20776, 31291, 43963, 56690,
    63344, 9676, 21244, 24773, 22024, 26047, 26197, 576, 1513, 4131, 407, 5460, 22493, 714, 717, 718, 44772, 59426,
    60974, 2062, 3501, 8097, 5304, 25954, 28344, 24828, 30352, 38222, 5046, 5818, 6686, 42742, 43388, 44816, 42636,
    44031, 46739, 10148, 21862, 44686, 8242, 52175, 62876, 4747, 32609, 34058, 6566, 7143, 8234, 14922, 15711, 18984,
    5824, 40590, 59342, 33682, 45072, 62057, 38428, 48178, 54214, 3077, 3116, 52444, 5573, 8663, 22129, 149, 210, 217,
    35360, 41515, 57033, 2983, 17389, 21704, 43536, 46688, 47878, 887, 977, 1323, 11412, 22316, 32347, 7869, 12860,
    17621, 4387, 15407, 54399, 24663, 36366, 44103, 33616, 39607, 55469, 4889, 4998, 5827, 26492, 32144, 58787, 15551,
    19916, 33553, 1249, 12786, 59280, 7420, 12309, 16108, 24073, 24129, 25182, 36368, 49085, 52779, 50520, 52249,
    62195, 26311, 38799, 40615, 2509, 4701, 5257, 4375, 6099, 7027, 1301, 1684, 1756, 27614, 44720, 53648, 403, 1300,
    12274, 4345, 5178, 7597, 7456, 15420, 16584, 30561, 31157, 54808, 24219, 25430, 25940, 5295, 8163, 10292, 269,
    277, 542, 47229, 52307, 52674, 94, 4137, 12599, 26440, 29924, 36363, 2321, 4494, 4873, 419, 3809, 5332, 952, 1332,
    7743, 22973, 32422, 33435, 31115, 31201, 32964, 2247, 15897, 55024, 18147, 48380, 48842, 1388, 5382, 9183, 30921,
    35577, 42566, 35907, 45770, 50605, 22403, 28108, 53212, 2419, 30041, 35605, 45334, 47012, 58578, 6942, 9813,
    17077, 12479, 19525, 25570, 5114, 5318, 7064, 3269, 16888, 21815, 6679, 9549, 22445, 9614, 31670, 34410, 23979,
    29951, 31804, 8441, 16343, 20713, 17381, 17672, 18025, 16393, 27843, 30638, 16355, 17825, 26641, 17043, 28101,
    28111, 15547, 24522, 31465, 8417, 8466, 9962, 27133, 46300, 48297, 5598, 6948, 9913, 43683, 46450, 47488, 12328,
    26168, 49748, 11729, 11795, 18289, 17304, 22263, 22424, 8344, 8416, 42425, 4359, 6317, 8772, 11139, 50974, 63075,
    3640, 14987, 17795, 975, 8767, 11582, 1465, 3669, 10241, 22151, 23387, 23497, 31031, 38662, 41462, 243, 474, 5216,
    16829, 20238, 20240, 13173, 13727, 17907, 49983, 50009, 50322, 1776, 14852, 19264, 4592, 5818, 9921, 4415, 7110,
    23579, 44770, 53588, 53622, 55639, 56012, 57935, 46600, 53863, 63707, 28991, 38380, 54903, 3420, 4620, 12746,
    15304, 17894, 21951, 55665, 56422, 56855
  ];

  let expected = [
    102, 70, 200, 135, 239, 117, 17, 3, 195, 97, 159, 84, 183, 139, 185, 97, 24, 90, 5, 2, 132, 60, 129, 176, 86, 130,
    222, 4, 8, 88, 28, 97, 92, 28, 54, 171, 159, 183, 229, 217, 186, 183, 207, 18, 240, 229, 27, 201, 109, 77, 188, 203,
    154, 231, 18, 175, 199, 82, 185, 25, 97, 202, 6, 174, 110, 165, 149, 23, 30, 162, 144, 218, 108, 143, 145, 109, 153,
    202, 69, 64, 218, 16, 172, 42, 171, 237, 64, 50, 76, 234, 149, 43, 16, 156, 93, 90, 6, 108, 55, 25, 70, 172, 234,
    199, 249, 19, 198, 233, 166, 95, 22, 197, 178, 103, 131, 113, 247, 63, 92, 226, 227, 91, 231, 168, 71, 31, 111, 152,
    111, 231, 163, 113, 96, 178, 86, 144, 104, 83, 160, 150, 8, 225, 51, 238, 89, 65, 67, 2, 86, 150, 64, 180, 114, 67,
    69, 59, 115, 73, 21, 91, 104, 89, 186, 156, 177, 209, 168, 10, 109, 58, 137, 96, 61, 198, 123, 67, 38, 209, 119, 41,
    145, 27, 223, 190, 59, 114, 10, 124, 117, 95, 208, 67, 18, 230, 120, 82, 51, 255, 90, 211, 146, 249, 212, 86, 114,
    4, 196, 138, 46, 183, 75, 42, 223, 109, 72, 230, 111, 186, 61, 60, 211, 204, 95, 248, 255, 153, 237, 220, 178, 45,
    217, 103, 206, 52, 21, 195, 54, 41, 75, 251, 86, 124, 95, 255, 16, 0, 207, 221, 67, 1, 254, 161, 93, 80, 239, 200,
    146, 118, 116, 110, 101, 206, 55, 202, 154, 217, 190, 223, 187, 232, 105, 179, 79, 131, 240, 13, 246, 144, 113, 110,
    233, 9, 147, 197, 124, 188, 229, 75, 18, 9, 99, 107, 151, 69, 221, 170, 102, 246, 101, 32, 205, 178, 153, 187, 143,
    157, 247, 28, 44, 168, 99, 46, 180, 158, 48, 110, 73, 48, 14, 21, 77, 16, 68, 3, 214, 23, 230, 188, 0, 223, 176, 12,
    64, 235, 62, 22, 82, 159, 239, 61, 105, 178, 10, 7, 35, 78, 167, 154, 50, 181, 255, 229, 103, 216, 190, 15
  ];

  let encoder = new RangeOptimalEncoder();
  for (let i = 0; i < triplets.length; i += 3) {
    encoder.encodeRange(triplets[i], triplets[i + 1], triplets[i + 2]);
  }
  let data = encoder.finish();

  assertArrayEquals(expected, data);

  let decoder = new RangeDecoder(data);
  for (let i = 0; i < triplets.length; i += 3) {
    let val = decoder.currentCount(triplets[i + 2]);
    assertTrue((val >= triplets[i]) && (val < triplets[i + 1]));
    decoder.removeRange(triplets[i], triplets[i + 1]);
  }
}

function testOptimizer() {
  let encoder = new RangeOptimalEncoder();
  encoder.encodeRange(1, 2, 257);
  let output = encoder.finish();
  assertArrayEquals([1], output);
}
