/* ProfileCreator V.3.6 (UAV) Jun 22 2012 */
/*  3.2: Only : CMin WMin WMax CMax */
/*  3.3: Add section VALUE, new PARAM struct */
/*  3.4: Add GRP_ACCESS (#MFLAG) */
/*  3.6: dynamic param colors to .value, ?_getvalue()*/
PARAM_INDEX param_ix[] = {
	{&pT0,NULL},
	{&pT1,NULL},
	{&pT2,NULL},
	{&pT3,NULL},
	{&pT4,NULL},
	{&pT5,NULL},
	{&pT6,NULL},
	{&pT7,NULL},
	{&pT8,NULL},
	{&pT9,NULL},
	{&pT10,NULL},
	{&pT11,NULL},
	{&pT12,NULL},
	{&pT13,NULL},
	{&pT14,NULL},
	{&pT15,NULL},
	{&pT16,NULL},
	{&pT17,NULL},
	{&pT18,NULL},
	{&pT19,NULL},
	{&pT20,NULL},
	{&pT21,NULL},
	{&pT22,NULL},
	{&pT23,NULL},
	{&pT24,NULL},
	{&pT25,NULL},
	{&pT26,NULL},
	{&pT27,NULL},
	{&pT28,NULL},
	{&pT29,NULL},
	{&pT30,NULL},
	{&pT31,NULL},
	{&pT32,NULL},
	{&pT33,NULL},
	{&pT34,NULL},
	{&pT35,NULL},
	{&pT36,NULL},
	{&pT37,NULL},
	{&pT38,NULL},
	{&pT39,NULL},
	{&pT40,NULL},
	{&pT41,NULL},
	{&pT42,NULL},
	{&pT43,NULL},
	{&pT44,NULL},
	{&pT45,NULL},
	{&pT46,NULL},
	{&pT47,NULL},
	{&pT48,NULL},
	{&pT49,NULL},
	{&pT50,NULL},
	{&pT51,NULL},
	{&pT52,NULL},
	{&pT53,NULL},
	{&pT54,NULL},
	{&pT55,NULL},
	{&pT56,NULL},
	{&pT57,NULL},
	{&pT58,NULL},
	{&pT59,NULL},
	{&pT60,NULL},
	{&pT61,NULL},
	{&pT62,NULL},
	{&pT63,NULL},
	{&pT64,NULL},
	{&pT65,NULL},
	{&pT66,NULL},
	{&pT67,NULL},
	{&pT68,NULL},
	{&pT69,NULL},
	{&pT70,NULL},
	{&pT71,NULL},
	{&pT72,NULL},
	{&pT73,NULL},
	{&pT74,NULL},
	{&pT75,NULL},
	{&pT76,NULL},
	{&pT77,NULL},
	{&pT78,NULL},
	{&pT79,NULL},
	{&pT80,NULL},
	{&pT81,NULL},
	{&pT82,NULL},
	{&pT83,NULL},
	{&pT84,NULL},
	{&pT85,NULL},
	{&pT86,NULL},
	{&pT87,NULL},
	{&pT88,NULL},
	{&pT89,NULL},
	{&pT90,NULL},
	{&pT91,NULL},
	{&pT92,NULL},
	{&pT93,NULL},
	{&pT94,NULL},
	{&pT95,NULL},
	{&pT96,NULL},
	{&pT97,NULL},
	{&pT98,NULL},
	{&pT99,NULL},
	{&pT100,NULL},
	{&pT101,NULL},
	{&pT102,NULL},
	{&pT103,NULL},
	{&pT104,NULL},
	{&pT105,NULL},
	{&pT106,NULL},
	{&pT107,NULL},
	{&pT108,NULL},
	{&pT109,NULL},
	{&pT110,NULL},
	{&pT111,NULL},
	{&pT112,NULL},
	{&pT113,NULL},
	{&pT114,NULL},
	{&pT115,NULL},
	{&pT116,NULL},
	{&pT117,NULL},
	{&pT118,NULL},
	{&pT119,NULL},
	{&pT120,NULL},
	{&pT121,NULL},
	{&pT122,NULL},
	{&pT123,NULL},
	{&pT124,NULL},
	{&pT125,NULL},
	{&pT126,NULL},
	{&pT127,NULL},
	{&pT128,NULL},
	{&pT129,NULL},
	{&pT130,NULL},
	{&pT131,NULL},
	{&pT132,NULL},
	{&pT133,NULL},
	{&pT134,NULL},
	{&pT135,NULL},
	{&pT136,NULL},
	{&pT137,NULL},
	{&pT138,NULL},
	{&pT139,NULL},
	{&pT140,NULL},
	{&pT141,NULL},
	{&pT142,NULL},
	{&pT143,NULL},
	{&pT144,NULL},
	{&pT145,NULL},
	{&pT146,NULL},
	{&pT147,NULL},
	{&pT148,NULL},
	{&pT149,NULL},
	{&pT150,NULL},
	{&pT151,NULL},
	{&pT152,NULL},
	{&pT153,NULL},
	{&pT154,NULL},
	{&pT155,NULL},
	{&pT156,NULL},
	{&pT157,NULL},
	{&pT158,NULL},
	{&pT159,NULL},
	{&pT160,NULL},
	{&pT161,NULL},
	{&pT162,NULL},
	{&pT163,NULL},
	{&pT164,NULL},
	{&pT165,NULL},
	{&pT166,NULL},
	{&pT167,NULL},
	{&pT168,NULL},
	{&pT169,NULL},
	{&pT170,NULL},
	{&pT171,NULL},
	{&pT172,NULL},
	{&pT173,NULL},
	{&pT174,NULL},
	{&pT175,NULL},
	{&pT176,NULL},
	{&pT177,NULL},
	{&pT178,NULL},
	{&pT179,NULL},
	{&pT180,NULL},
	{&pT181,NULL},
	{&pT182,NULL},
	{&pT183,NULL},
	{&pT184,NULL},
	{&pT185,NULL},
	{&pT186,NULL},
	{&pT187,NULL},
	{&pT188,NULL},
	{&pT189,NULL},
	{&pT190,NULL},
	{&pT191,NULL},
	{&pT192,NULL},
	{&pT193,NULL},
	{&pT194,NULL},
	{&pT195,NULL},
	{&pT196,NULL},
	{&pT197,NULL},
	{&pT198,NULL},
	{&pT199,NULL},
	{&pT200,NULL},
	{&pT201,NULL},
	{&pT202,NULL},
	{&pT203,NULL},
	{&pT204,NULL},
	{&pT205,NULL},
	{&pT206,NULL},
	{&pT207,NULL},
	{&pT208,NULL},
	{&pT209,NULL},
	{&pT210,NULL},
	{&pT211,NULL},
	{&pT212,NULL},
	{&pT213,NULL},
	{&pT214,NULL},
	{&pT215,NULL},
	{&pT216,NULL},
	{&pT217,NULL},
	{&pT218,NULL},
	{&pT219,NULL},
	{&pT220,NULL},
	{&pT221,NULL},
	{&pT222,NULL},
	{&pT223,NULL},
	{&pT224,NULL},
	{&pT225,NULL},
	{&pT226,NULL},
	{&pT227,NULL},
	{&pT228,NULL},
	{&pT229,NULL},
	{&pT230,NULL},
	{&pT231,NULL},
	{&pT232,NULL},
	{&pT233,NULL},
	{&pT234,NULL},
	{&pT235,NULL},
	{&pT236,NULL},
	{&pT237,NULL},
	{&pT238,NULL},
	{&pT239,NULL},
	{&pT240,NULL},
	{&pT241,NULL},
	{&pT242,NULL},
	{&pT243,NULL},
	{&pT244,NULL},
	{&pT245,NULL},
	{&pT246,NULL},
	{&pT247,NULL},
	{&pT248,NULL},
	{&pT249,NULL},
	{&pT250,NULL},
	{&pT251,NULL},
	{&pT252,NULL},
	{&pT253,NULL},
	{&pT254,NULL},
	{&pT255,NULL},
	{&pT256,NULL},
	{&pT257,NULL},
	{&pT258,NULL},
	{&pT259,NULL},
	{&pT260,NULL},
	{&pT261,NULL},
	{&pT262,NULL},
	{&pT263,NULL},
	{&pT264,NULL},
	{&pT265,NULL},
	{&pT266,NULL},
	{&pT267,NULL},
	{&pT268,NULL},
	{&pT269,NULL},
	{&pT270,NULL},
	{&pT271,NULL},
	{&pT272,NULL},
	{&pT273,NULL},
	{&pT274,NULL},
	{&pT275,NULL},
	{&pT276,NULL},
	{&pT277,NULL},
	{&pT278,NULL},
	{&pT279,NULL},
	{&pT280,NULL},
	{&pT281,NULL},
	{&pT282,NULL},
	{&pT283,NULL},
	{&pT284,NULL},
	{&pT285,NULL},
	{&pT286,NULL},
	{&pT287,NULL},
	{&pT288,NULL},
	{&pT289,NULL},
	{&pT290,NULL},
	{&pT291,NULL},
	{&pT292,NULL},
	{&pT293,NULL},
	{&pT294,NULL},
	{&pT295,NULL},
	{&pT296,NULL},
	{&pT297,NULL},
	{&pT298,NULL},
	{&pT299,NULL},
	{&pT300,NULL},
	{&pT301,NULL},
	{&pT302,NULL},
	{&pT303,NULL},
	{&pT304,NULL},
	{&pT305,NULL},
	{&pT306,NULL},
	{&pT307,NULL},
	{&pT308,NULL},
	{&pT309,NULL},
	{&pT310,NULL},
	{&pT311,NULL},
	{&pT312,NULL},
	{&pT313,NULL},
	{&pT314,NULL},
	{&pT315,NULL},
	{&pT316,NULL},
	{&pT317,NULL},
	{&pT318,NULL},
	{&pT319,NULL},
	{&pT320,NULL},
	{&pT321,NULL},
	{&pT322,NULL},
	{&pT323,NULL},
	{&pT324,NULL},
	{&pT325,NULL},
	{&pT326,NULL},
	{&pT327,NULL},
	{&pT328,NULL},
	{&pT329,NULL},
	{&pT330,NULL},
	{&pT331,NULL},
	{&pT332,NULL},
	{&pT333,NULL},
	{&pT334,NULL},
	{&pT335,NULL},
	{&pT336,NULL},
	{&pT337,NULL},
	{&pT338,NULL},
	{&pT339,NULL},
	{&pT340,NULL},
	{&pT341,NULL},
	{&pT342,NULL},
	{&pT343,NULL},
	{&pT344,NULL},
	{&pT345,NULL},
	{&pT346,NULL},
	{&pT347,NULL},
	{&pT348,NULL},
	{&pT349,NULL},
	{&pT350,NULL},
	{&pT351,NULL},
	{&pT352,NULL},
	{&pT353,NULL},
	{&pT354,NULL},
	{&pT355,NULL},
	{&pT356,NULL},
	{&pT357,NULL},
	{&pT358,NULL},
	{&pT359,NULL},
	{&pT360,NULL},
	{&pT361,NULL},
	{&pT362,NULL},
	{&pT363,NULL},
	{&pT364,NULL},
	{&pT365,NULL},
	{&pT366,NULL},
	{&pT367,NULL},
	{&pT368,NULL},
	{&pT369,NULL},
	{&pT370,NULL},
	{&pT371,NULL},
	{&pT372,NULL},
	{&pT373,NULL},
	{&pT374,NULL},
	{&pT375,NULL},
	{&pT376,NULL},
	{&pT377,NULL},
	{&pT378,NULL},
	{&pT379,NULL},
	{&pT380,NULL},
	{&pT381,NULL},
	{&pT382,NULL},
	{&pT383,NULL},
	{&pT384,NULL},
	{&pT385,NULL},
	{&pT386,NULL},
	{&pT387,NULL},
	{&pT388,NULL},
	{&pT389,NULL},
	{&pT390,NULL},
	{&pT391,NULL},
	{&pT392,NULL},
	{&pT393,NULL},
	{&pT394,NULL},
	{&pT395,NULL},
	{&pT396,NULL},
	{&pT397,NULL},
	{&pT398,NULL},
	{&pT399,NULL},
	{&pT400,NULL},
	{&pT401,NULL},
	{&pT402,NULL},
	{&pT403,NULL},
	{&pT404,NULL},
	{&pT405,NULL},
	{&pT406,NULL},
	{NULL}
};