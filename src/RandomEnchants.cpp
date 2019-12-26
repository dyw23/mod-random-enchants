#include "ScriptMgr.h"
#include "Configuration/Config.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Player.h"
#include "Object.h"

bool RandomEnchantEnabled ;
bool RandomEnchantAnnounce ;
bool Crafted ;
bool Looted ;
bool QuestReward ;
uint32 HighQuality ;
uint32 LowQuality ;
uint32 Chance1 ;
uint32 Chance2 ;
uint32 Chance3 ;

class REConfig : public WorldScript
{
public:
    REConfig() : WorldScript("REConfig") { }

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload) {
            std::string conf_path = _CONF_DIR;
            std::string cfg_file = conf_path + "/mod_randomenchants.conf";

            std::string cfg_def_file = cfg_file + ".dist";
            sConfigMgr->LoadMore(cfg_def_file.c_str());
            sConfigMgr->LoadMore(cfg_file.c_str());

            // Load Configuration Settings
            SetInitialWorldSettings();
        }
    }

    // Load Configuration Settings
    void SetInitialWorldSettings()
    {

        // Sanitize
        if (HighQuality > 5) { HighQuality = 5; }
		
    }
};

class RandomEnchantsPlayer : public PlayerScript {
public:

    RandomEnchantsPlayer() : PlayerScript("RandomEnchantsPlayer") { }

    void OnLogin(Player* player)  override
    {
        if (sConfigMgr->GetBoolDefault("RandomEnchants.Announce", true)) {
            ChatHandler(player->GetSession()).SendSysMessage("This server is running the |cff4CFF00RandomEnchants |rmodule.");
        }
    }

    void OnLootItem(Player* player, Item* item, uint32 /*count*/, uint64 /*lootguid*/) override
    {
        if (RandomEnchantEnabled)
        {
            RollPossibleEnchant(player, item);
        }
    }

    void OnCreateItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (RandomEnchantEnabled)
        {
            if (Crafted == true)
            {
                RollPossibleEnchant(player, item);
            }
        }
    }

    void OnQuestRewardItem(Player* player, Item* item, uint32 /*count*/) override
    {
        if (RandomEnchantEnabled)
        {
            if (QuestReward == true)
            {
                RollPossibleEnchant(player, item);
            }
        }
    }

    void RollPossibleEnchant(Player* player, Item* item)
    {
        uint32 Quality = item->GetTemplate()->Quality;
        uint32 Class = item->GetTemplate()->Class;

        // Weed out items ( needs more testing )
        if ((Quality < LowQuality && Quality > HighQuality) || (Class != 2 && Class != 4))
        {
            // Eliminate enchanting anything that isn't a designated quality.
            // Eliminate enchanting anything but weapons(2) and armor(4).
            return;
        }

        // It's what we want..
        int slotRand[3] = { -1, -1, -1 };
        uint32 slotEnch[3] = { 0, 1, 5 };
        double roll1 = rand_chance();
        if (roll1 >= Chance1)
            slotRand[0] = getRandEnchantment(item);
        if (slotRand[0] != -1)
        {
            double roll2 = rand_chance();
            if (roll2 >= Chance2)
                slotRand[1] = getRandEnchantment(item);
            if (slotRand[1] != -1)
            {
                double roll3 = rand_chance();
                if (roll3 >= Chance3)
                    slotRand[2] = getRandEnchantment(item);
            }
        }
        for (int i = 0; i < 3; i++)
        {
            if (slotRand[i] != -1)
            {
                if (sSpellItemEnchantmentStore.LookupEntry(slotRand[i])) //Make sure enchantment id exists
                {
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), false);
                    item->SetEnchantment(EnchantmentSlot(slotEnch[i]), slotRand[i], 0, 0);
                    player->ApplyEnchantment(item, EnchantmentSlot(slotEnch[i]), true);
                }
            }
        }
        ChatHandler chathandle = ChatHandler(player->GetSession());
        if (slotRand[2] != -1)
            chathandle.PSendSysMessage("|cffDA70D6 The |cff71C671%s |cffDA70D6is marked with ancient runes that emit a radiant glow.", item->GetTemplate()->Name1.c_str());
        else if (slotRand[1] != -1)
            chathandle.PSendSysMessage("|cffDA70D6 The |cff71C671%s |cffDA70D6glows brightly as you pick it up.", item->GetTemplate()->Name1.c_str());
        else if (slotRand[0] != -1)
            chathandle.PSendSysMessage("|cffDA70D6 The |cff71C671%s |cffDA70D6is clearly a cut above the rest.", item->GetTemplate()->Name1.c_str());
    }

    int getRandEnchantment(Item* item)
    {
        uint32 Class = item->GetTemplate()->Class;
        std::string ClassQueryString = "";
        switch (Class)
        {
        case 2:
            ClassQueryString = "WEAPON";
            break;
        case 4:
            ClassQueryString = "ARMOR";
            break;
        }
        if (ClassQueryString == "")
            return -1;
        uint32 Quality = item->GetTemplate()->Quality;
        int rarityRoll = -1;
        switch (Quality)
            {
        case 0://grey
            rarityRoll = rand_norm() * 14;
            break;
        case 1://white
            rarityRoll = rand_norm() * 15;
            break;
        case 2://green
            rarityRoll = 45 + (rand_norm() * 15);
            break;
        case 3://blue
            rarityRoll = 65 + (rand_norm() * 25);
            break;
        case 4://purple
            rarityRoll = 75 + (rand_norm() * 50);
            break;
        case 5://orange
            rarityRoll = 85 + (rand_norm() * 75);
            break;
            }
        if (rarityRoll < 0)
            return -1;
        int tier = 0;
        if (rarityRoll <= 44)
            tier = 1;
        else if (rarityRoll <= 64)
            tier = 2;
        else if (rarityRoll <= 74)
            tier = 3;
        else if (rarityRoll <= 84)
            tier = 4;
        else
            tier = 5;

        QueryResult qr = WorldDatabase.PQuery("SELECT enchantID FROM item_enchantment_random_tiers WHERE tier='%d' AND exclusiveSubClass=NULL AND class='%s' OR exclusiveSubClass='%u' OR class='ANY' ORDER BY RAND() LIMIT 1", tier, ClassQueryString.c_str(), item->GetTemplate()->SubClass);
        return qr->Fetch()[0].GetUInt32();
    }
};

void AddRandomEnchantsScripts()
{
    new REConfig();
    new RandomEnchantsPlayer();
}
