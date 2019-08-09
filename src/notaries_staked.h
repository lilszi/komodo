
#ifndef NOTARIES_STAKED
#define NOTARIES_STAKED

#include "crosschain.h"
#include "cc/CCinclude.h"

static const int32_t iguanaPort = 9333;
static const int8_t BTCminsigs = 13;
static const int8_t overrideMinSigs = 7;
static const char *iguanaSeeds[8][1] =
{
  {"94.23.1.95"},
  {"103.6.12.112"},
  {"18.224.176.46"},
  {"45.76.120.247"},
  {"185.62.57.32"},
  {"149.28.253.160"},
  {"68.183.226.124"},
  {"149.28.246.230"},
};

static const int STAKED_ERA_GAP = 777;

static const int NUM_STAKED_ERAS = 4;
static const int STAKED_NOTARIES_TIMESTAMP[NUM_STAKED_ERAS] = {1604244444, 1604244444, 1604244444, 1604244444};
static const int32_t num_notaries_STAKED[NUM_STAKED_ERAS] = { 59, 1, 1, 1 };

/* Era array of pubkeys.
static const char *notaries_STAKED[NUM_STAKED_ERAS][64][2] =
{
    {
        {"blackjok3r", "035fc678bf796ad52f69e1f5759be54ec671c559a22adf27eed067e0ddf1531574" }, // RTcYRJ6WopYkUqcmksyjxoV1CueYyqxFuk    right
        {"Alright", "02b718c60a035f77b7103a507d36aed942b4f655b8d13bce6f28b8eac523944278" }, //RG77F4mQpP1K1q2CDSc2vZSJvKUZgF8R26
        {"webworker01", "031d1fb39ae4dca28965c3abdbd21faa0f685f6d7b87a60561afa7c448343fef6d" }, //RGsQiArk5sTmjXZV9UzGMW5njyvtSnsTN8    right
        {"CrisF", "03745656c8991c4597828aad2820760c43c00ff2e3b381fef3b5c040f32a7b3a34" }, // RNhYJAaPHJCVXGWNVEJeP3TfepEPdhjrRr         right
        {"smk762", "02381616fbc02d3f0398c912fe7b7daf2f3f29e55dc35287f686b15686d8135a9f" }, // RSchwBApVquaG6mXH31bQ6P83kMN4Hound        right
        {"jorian", "0343eec31037d7b909efd968a5b5e7af60321bf7e464da28f815f0fb23ee7aadd7" }, // RJorianBXNwfNDYPxtNYJJ6vX7Z3VdTR25        right
        {"TonyL", "021a559101e355c907d9c553671044d619769a6e71d624f68bfec7d0afa6bd6a96" }, // RHq3JsvLxU45Z8ufYS6RsDpSG4wi6ucDev
        {"CHMEX", "03ed125d1beb118d12ff0a052bdb0cee32591386d718309b2924f2c36b4e7388e6" }, // RF4HiVeuYpaznRPs7fkRAKKYqT5tuxQQTL         right 
        {"metaphilibert", "0344182c376f054e3755d712361672138660bda8005abb64067eb5aa98bdb40d10" }, // RG28QSnYFADBg1dAVkH1uPGYS6F8ioEUM2 right
        {"gt", "02312dcecb6e4a32927a075972d3c009f3c68635d8100562cc1813ea66751b9fde" }, // RCg4tzKWQ7i3wrZEU8bvCbCQ4xRJnHnyoo            right
        {"CMaurice", "026c6d094523e810641b89f2d7f0ddd8f0b59d97c32e1fa97f0e3e0ac119c26ae4" }, // RSjayeSuYUE1E22rBjnqoexobaRjbAZ2Yb      right
        {"Bar_F1sh_Rel", "0395f2d9dd9ccb78caf74bff49b6d959afb95af746462e1b35f4a167d8e82b3666" }, // RBbLxJagCA9QHDazQvfnDZe874V1K4Gu8t
        {"zatJUM", "030fff499b6dc0215344b28a0b6b4becdfb00cd34cd1b36b983ec14f47965fd4bc" }, // RSoEDLBasth7anxS8gbkg6KgeGiz8rhqv1        right
        {"dwy", "03669457b2934d98b5761121dd01b243aed336479625b293be9f8c43a6ae7aaeff" }, // RKhZMqRF361FSGFAzstP5AhozekPjoVh5q
        {"gcharang", "021569dd350d99e685a739c5b36bd01f217efb4f448a6f9a56da80c5edf6ce20ee" }, // RE8SsNwhYoygXJSvw9DuQbJicDc28dwR78      right
        {"computergenie", "027313dabde94fb72f823231d0a1c59fc7baa2e5b3bb2af97ca7d70aae116026b9" }, // RLabsCGxTRqJcJvz6foKuXAB61puJ2x8yt right
        {"daemonfox", "0383484bdc745b2b953c85b5a0c496a1f27bc42ae971f15779ed1532421b3dd943" }, //
        {"SHossain", "02791f5c215b8a19c143a98e3371ff03b5613df9ac430c4a331ca55fed5761c800" }, // RKdLoHkyeorXmMtj91B1AAnAGiwsdt9MdF
        {"Nabob", "03ee91c20b6d26e3604022f42df6bb8de6f669da4591f93b568778cba13d9e9ddf" }, // RRwCLPZDzpHEFJnLev4phy51e2stHRUAaU
        {"mylo", "03f6b7fcaf0b8b8ec432d0de839a76598b78418dadd50c8e5594c0e557d914ec09" }, // RXN4hoZkhUkkrnef9nTUDw3E3vVALAD8Kx
        {"PHBA2061", "039fc98c764bc85aed97d690d7942a4fd1190b2fa4f5f4c5c8e0957fac5c6ede00" }, // RPHba2o61hcpX4ds91oj3sKJ8aDXv6QdQf      right
        {"Exile13", "0247b2120a39faf83678b5de6883e039180ff42925bcb298d32f3792cd59001aae" }, // RTDJ3CDZ6ANbeDKab8nqTVrGw7ViAKLeDV       right
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    }
}; */

// Era array of pubkeys.
static const char *notaries_STAKED[NUM_STAKED_ERAS][64][2] =
{
    {
        {"test0", "03b6138d95e4524ac18b511f97d49aedc0922e55fb3bd29195b68082ad7c612589" },
        {"test1", "03555fc8af271bb759fdf3a58456632ff5ffd88490daacbba1e55eb1454b26f88b" },
        {"test2", "03cfe87c4aa1ebd6b898b216ea3f3ca166a57fc0d5e5cb9446225fbf67239dbd0a" },
        {"test3", "033a8ced4ac89d5e68538ecbb2830d8346b88811d9ecb688ff894fbce5c7a0d727" },
        {"test4", "02ab7eb334eaa83a13d7d4b359ef655d24c64782d637889a6dc163be7f0479b5da" },
        {"test5", "0218ce501d2d099384f8ddec33664c6638a08da374264dbfe93144ec89080fae0f" },
        {"test6", "02a0c097c4fce5af7da38174882903e5a055c99daacf354cc5b1c5c85f57a4d408" },
        {"test7", "03780207528edb82d38de705659eaabe633326f376d9ced29e23d87b60858b7cdb" },
        {"test8", "02e3b902de67e2439a70facf6f815116897cd9312eda3622f9e398b73cd24c8223" },
        {"test9", "038e3589f8145ad4d2de671997df9d21961c89f2d9f9add802ed1cd30130e0e884" },
        {"test10", "03cfb935ff65cc1d04e9dbbe373cdf4f977c38fca9d96c1c29f4c14f2227f71993" },
        {"test11", "02558c6ff951adbf5ba35cbfceda57d315e4c552d3e854938cecdc68360945c78b" },
        {"test12", "03ee63e4348211f7c0d3e68dd8c788f7a11b83c3c7a17853428fe8c2a0c134745f" },
        {"test13", "0299ad6346bd0fc4e9ee398012fdad212978aacc7654101271a25fba65c6494c81" },
        {"test14", "02395141bf6bb2f7bdf52c9181f097c6c4bd7a3d9e011b9b0617ce34f025670609" },
        {"test15", "02d2f4d28e557b7ab3895291599be83e48e9ce5e5c8f6def6a232b10a9058de225" },
        {"test16", "030873f7cc02a96e46d88991e4bd8dc4effba8b24c4056d90588f683b0fe25e3ec" },
        {"test17", "027749a5b09f5dad072d977e71f61efc3fc673e4c2a146d8719a730c495a0afc08" },
        {"test18", "0395ac50e77f0eb5c9b990b1eb1fc40578bd04370664c4b24da2fb345aa2da3aa3" },
        {"test19", "03cd70778b6d463c5b643bd59141d7dbaa39070292c4e47d7092ca11f292f27080" },
        {"test20", "024814286a6baa92bfef939789b4ae5eab9f2861b22e7ceeea77ace0a83582a36c" },
        {"test21", "024f3b414e75465cab64b6c879f81a2b4429b9e1c9afe4f10a8142fe6d4199f4d4" },
        {"test22", "02bd149a5a07a6f2367cd68379a1effb8c2b23b66abe8ab5ca540e7147b82aded1" },
        {"test23", "02eef784ec27d6235110a7ef7ec9da8703d1457a2e5860b394a713c27510fb8bc3" },
        {"test24", "02cd1077b7fde5d33351df4bf47b48b0fbdb9d3d908bb898a4e9fd717244a324ae" },
        {"test25", "03e4c3faa95f7670a097b605b30200e8c0690757227f48c74408c04d2fac0eebb2" },
        {"test26", "032524acb5634b39564522452aa1a0a984c7a985554e27203c919b6930fe07f587" },
        {"test27", "02eb43b1b15d071660be1691f037ba1f94770bcd52f9dbfd0a1d06a4c551c4e21e" },
        {"test28", "0376c926efa6538096de04d99a785dbe52a33bf41a161e661346b6683806134dcd" },
        {"test29", "03f924e018c8bd483cd7589269641bd391eb2b6453d8b95d8e30d135f418c08b9d" },
        {"test30", "02b9d8a21d6e0c0c08fad5806ae1fc28d12f88de88924e6ab0055301c7029b9351" },
        {"test31", "036cd1d0c6f73023d483176c94e8fe74a1de16c549050db1b574081a9969d280f8" },
        {"test32", "03c2cf0c2360c4931963ce2a40c33db8c9f13179cb8f68e67ecc3ac1acb2000ef4" },
        {"test33", "02eb348ce25b32e359fbac9c177777edb863e37997e558d6654ce28b6379c7c18f" },
        {"test34", "02d4ce3256c2f2aa15e95fdad053157c4007b93d8d65b6f3c25bf8a23ad6b05286" },
        {"test35", "036923487d44024b375c3f73d30ee9c7cc7e10077667d2f6ba59277b1b16cc0973" },
        {"test36", "039bcf495cc5c571e598a90ed1b949e957baaa14700de34ffc1aacd00b4d90a61e" },
        {"test37", "031943efeebb25a45c1b539a71aaf50199126827da21078247ab7a35c97af0e7bb" },
        {"test38", "032529d624baf29681cc47db9a4f168898f2b6d8df65ce141f06b121afe2acb584" },
        {"test39", "0273d69cfefb193661eb8f306957ae80d948c7f3e1df3ec47517a139f7c8f47199" },
        {"test40", "02f79d09e1c3c12b72b0f64feda67bd3d1f2417e2035522a1693fa6b89952ed809" },
        {"test41", "024cea94122bb1c9d12640f904a4cb789a184c414c8248f04a0e4d41b40c4df0e2" },
        {"test42", "0367e8126b8ec66ddacbd8bfd0aa48eaed9c15d10132e2262deb49d8dccb7fac4b" },
        {"test43", "032bc5b2ca27d146d4c828f9c3ee4e240c5c9d7e91206b5b122749ec85c332b7f1" },
        {"test44", "021c34a51a2c42c776aeaec53a910755cc4749ddb13dff8299d5c21b779e7e3522" },
        {"test45", "02e503b7c87dc8fb6c63035576710ac59f5485bfab814f3e71b79612fde9a36503" },
        {"test46", "02b6bfc2952d92cf2d8d21a375d0709084d543582d874bb80cc7216b53abef7b14" },
        {"test47", "03f8312b8a604181f6105a93e8ee4124529a2d4da421d40da2cff3eefc8dab88a7" },
        {"test48", "0273a0393131d9771554b0b41bcd63cb6def1393976aa9c294b729abff0971f002" },
        {"test49", "0236d94e78cab0907a2a46422c47af18e3c01d29d9689fed16d18f67e2010ff959" },
        {"test50", "02867d93dee04b906815940e9c60ef875a7e95f65e135993931e057a4d70a11e33" },
        {"test51", "02086ca27fefd53a24c5202cbf68aa942fe0e63852654112d6931e8b76723426cf" },
        {"test52", "026e2e074ffb062daf441abf7d2107b6d3b2cf3569924651d7dfb6d14bc814d9c7" },
        {"test53", "02f9e4809b288d604c3b171fdc4e72dce67e60c431b0c0e4db5be269ce32c10bfd" },
        {"test54", "02f405f90f1cfe1df4749b19fb1881a757d98a4aeb103e01994554250af342a0de" },
        {"test55", "0216f03688c6b87f76dadf687594d678392a1937aba551598b122282d29fcb8de2" },
        {"test56", "037882c7cf559b46ae9397e0894cd96be236245d3ead7f042bdbe0bf42c697c972" },
        {"test57", "02f93c559a62cee528f87826f8c8f5bd423d47f137c04493c3e1ea8db1c040d647" },
        {"test58", "03ab86d4288f150274dd386c629284e94cde3c356eaca296b0b4a3587fd9e03910" }
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    },
    {
        {"blackjok3r", "021914947402d936a89fbdd1b12be49eb894a1568e5e17bb18c8a6cffbd3dc106e" }, // RTVti13NP4eeeZaCCmQxc2bnPdHxCJFP9x
    }
};

int8_t is_STAKED(const char *chain_name);
int32_t STAKED_era(int timestamp);
int8_t numStakedNotaries(uint8_t pubkeys[64][33],int8_t era);
int8_t StakedNotaryID(std::string &notaryname, char *Raddress);
void UpdateNotaryAddrs(uint8_t pubkeys[64][33],int8_t numNotaries);

CrosschainAuthority Choose_auth_STAKED(int32_t chosen_era);

#endif
