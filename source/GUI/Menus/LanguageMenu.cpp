/*
* NEWorld: A free game with similar rules to Minecraft.
* Copyright (C) 2017-2018 NEWorld Team
*
* This file is part of NEWorld.
* NEWorld is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* NEWorld is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with NEWorld.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Menus.h"
#include <deque>
#include <fstream>

struct Langinfo {
    std::string Symbol, EngSymbol, Name;
    GUI::button* Button;
};

namespace Menus {
    class Language : public GUI::Form {
    private:
        std::deque<Langinfo> Langs;
        GUI::label title;
        GUI::button backbtn;

        void onLoad() override {
            Langs.clear();
            title = GUI::label(GetStrbyKey("NEWorld.language.caption"), -225, 225, 20, 36, 0.5, 0.5, 0.0, 0.0);
            backbtn = GUI::button(GetStrbyKey("NEWorld.language.back"), -250, 250, -44, -20, 0.5, 0.5, 1.0, 1.0);
            registerControls(2, &title, &backbtn);
            std::ifstream index("Lang/Langs.txt");
            int count;
            index >> count;
            Langinfo Info;
            for (int i = 0; i < count; i++) {
                index >> Info.Symbol;
                std::ifstream LF("Lang/" + Info.Symbol + ".lang");
                std::getline(LF, Info.EngSymbol);
                std::getline(LF, Info.Name);
                LF.close();
                Info.Button = new GUI::button(Info.EngSymbol + "--" + Info.Name, -200, 200, i * 36 + 42, i * 36 + 72,
                                              0.5, 0.5, 0.0, 0.0);
                registerControls(1, Info.Button);
                Langs.push_back(Info);
            }
            index.close();
        }

        void onUpdate() override {
            AudioSystem::SpeedOfSound = AudioSystem::Air_SpeedOfSound;
            EFX::EAXprop = Generic;
            EFX::UpdateEAXprop();
            float Pos[] = {0.0f, 0.0f, 0.0f};
            AudioSystem::Update(Pos, false, false, Pos, false, false);
            if (backbtn.clicked) GUI::PopPage();
            for (auto& Lang : Langs) {
                if (Lang.Button->clicked) {
                    GUI::PopPage();
                    if (Globalization::Cur_Lang != Lang.Symbol) {
                        Globalization::LoadLang(Lang.Symbol);
                        GUI::BackToMain();
                    }
                    break;
                }
            }
        }

        void onLeave() override {
            for (auto& Lang : Langs) {
                for (std::vector<GUI::controls*>::iterator iter = children.begin(); iter != children.end();) {
                    if ((*iter)->id == Lang.Button->id) iter = children.erase(iter);
                    else ++iter;
                }
                Lang.Button->destroy();
                delete Lang.Button;
            }
        }
    };

    void languagemenu() { PushPage(new Language); }
}
