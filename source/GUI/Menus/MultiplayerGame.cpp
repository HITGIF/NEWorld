// 
// NEWorld: MultiplayerGame.cpp
// NEWorld: A Free Game with Similar Rules to Minecraft.
// Copyright (C) 2015-2018 NEWorld Team
// 
// NEWorld is free software: you can redistribute it and/or modify it 
// under the terms of the GNU Lesser General Public License as published
// by the Free Software Foundation, either version 3 of the License, or 
// (at your option) any later version.
// 
// NEWorld is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General 
// Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with NEWorld.  If not, see <http://www.gnu.org/licenses/>.
// 

#include "Menus.h"

namespace Menus {
    int getDotCount(const std::string& s) {
        int ret = 0;
        for (unsigned int i = 0; i != s.size(); i++)
            if (s[i] == '.') ret++;
        return ret;
    }

    class MultiplayerMenu : public GUI::Form {
    private:
        GUI::label title;
        GUI::textbox serveriptb;
        GUI::button runbtn, okbtn, backbtn;

        void onLoad() override {
            title = GUI::label("==============<  多 人 游 戏  >==============", -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            serveriptb = GUI::textbox("输入服务器IP", -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            runbtn = GUI::button("运行服务器（开服）", -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            okbtn = GUI::button("确定", -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            backbtn = GUI::button("<< 返回", -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            inputstr = "";
            okbtn.enabled = false;
            registerControls(4, &title, &serveriptb, &runbtn, &okbtn, &backbtn);
        }

        void onUpdate() override {
            static bool serveripChanged = false;
#ifdef NEWORLD_USE_WINAPI
            if (runbtn.clicked) WinExec("NEWorldServer.exe", SW_SHOWDEFAULT);
#endif
            if (okbtn.clicked) {
                serverip = serveriptb.text;
                gamebegin = true;
                multiplayer = true;
            }

            AudioSystem::SpeedOfSound = AudioSystem::Air_SpeedOfSound;
            EFX::EAXprop = Generic;
            EFX::UpdateEAXprop();
            float Pos[] = {0.0f, 0.0f, 0.0f};
            AudioSystem::Update(Pos, false, false, Pos, false, false);
            if (backbtn.clicked) GUI::PopPage();
            if (serveriptb.pressed && !serveripChanged) {
                serveriptb.text = "";
                serveripChanged = true;
            }
            if (serveriptb.text.empty() || !serveripChanged || getDotCount(serveriptb.text) != 3) okbtn.enabled = false;
            else okbtn.enabled = true;
            inputstr = "";
        }
    };

    void multiplayermenu() { PushPage(new MultiplayerMenu); }
}
