#include "Hooks.hpp"
#include "../Logic/logic.hpp"
#include <chrono>
#include "../Hack/audiopitchHack.hpp"
#include "../Logic/autoclicker.hpp"
#include "../GUI/gui.hpp"
#include "../Logic/speedhack.h"

#define FRAME_LABEL_ID 82369 + 1 //random value :P
#define CPS_LABEL_ID 82369 + 2 //random value :P
#define CPS_BREAKS_LABEL_ID 82369 + 3 //random value :P
#define RECORDING_LABEL_ID 82369 + 4 //random value :P
#define PERCENT_LABEL_ID 82369 + 5 //random value :P
#define TIME_LABEL_ID 82369 + 6 //random value :P

#define HOOK(o, f) MH_CreateHook(reinterpret_cast<void*>(gd::base + o), f##_h, reinterpret_cast<void**>(&f));
// gracias matcool :]

#define HOOK_COCOS(o, f) MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), o), f##_h, reinterpret_cast<void**>(&f));

void Hooks::init_hooks() {
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1fb780), PlayLayer::init_h, reinterpret_cast<void**>(&PlayLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x2029C0), PlayLayer::update_h, reinterpret_cast<void**>(&PlayLayer::update));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111500), PlayLayer::pushButton_h, reinterpret_cast<void**>(&PlayLayer::pushButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x111660), PlayLayer::releaseButton_h, reinterpret_cast<void**>(&PlayLayer::releaseButton));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20BF00), PlayLayer::resetLevel_h, reinterpret_cast<void**>(&PlayLayer::resetLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20DDD0), createCheckpoint_h, reinterpret_cast<void**>(&createCheckpoint));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20b830), removeCheckpoint_h, reinterpret_cast<void**>(&removeCheckpoint));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1E4570), PauseLayer::init_h, reinterpret_cast<void**>(&PauseLayer::init));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20D810), PlayLayer::exitLevel_h, reinterpret_cast<void**>(&PlayLayer::exitLevel));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x1f4ff0), PlayerObject_ringJump_h, reinterpret_cast<void**>(&PlayerObject_ringJump));
    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0xef0e0), GameObject_activateObject_h, reinterpret_cast<void**>(&GameObject_activateObject));

    MH_CreateHook(reinterpret_cast<void*>(gd::base + 0x20A1A0), PlayLayer::death_h, reinterpret_cast<void**>(&PlayLayer::death));

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?dispatchKeyboardMSG@CCKeyboardDispatcher@cocos2d@@QAE_NW4enumKeyCodes@2@_N@Z"), CCKeyboardDispatcher_dispatchKeyboardMSG_h, reinterpret_cast<void**>(&CCKeyboardDispatcher_dispatchKeyboardMSG));;

    HOOK(0x205460, PlayLayer::updateVisibility)

    MH_CreateHook(GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?update@CCScheduler@cocos2d@@UAEXM@Z"), CCScheduler_update_h, reinterpret_cast<void**>(&CCScheduler_update));

    MH_EnableHook(MH_ALL_HOOKS);
}

bool g_disable_render = false;
float g_left_over = 0.f;

bool __fastcall Hooks::PlayLayer::death_h(void* self, void*, void* go, void* thingy) {
    auto& logic = Logic::get();

    if (gd::GameManager::sharedState()->getPlayLayer()->m_player1 == go && logic.noclip_player1) {
        return 0; 
    }

    if (gd::GameManager::sharedState()->getPlayLayer()->m_isDualMode) {
        if (gd::GameManager::sharedState()->getPlayLayer()->m_player2 == go && logic.noclip_player2) {
            return 0;
        }
    }

    return Hooks::PlayLayer::death(self, go, thingy);

}

std::mutex mtx;
void __fastcall Hooks::CCScheduler_update_h(CCScheduler* self, int, float dt) {
    auto& logic = Logic::get();
    auto play_layer = gd::GameManager::sharedState()->getPlayLayer();

    if (logic.frame_advance) return;

    if (GUI::get().change_display_fps)
        CCDirector::sharedDirector()->setAnimationInterval(1.f / GUI::get().input_fps);

    if (logic.recorder.m_recording && !logic.recorder.real_time_rendering) {
        dt = 1.f / logic.fps;
        Speedhack::SetSpeed(0.5);
        return CCScheduler_update(self, dt);
    } 

    Speedhack::SetSpeed(1);

    if (logic.autoclicker && play_layer && !play_layer->m_isPaused) {
        Autoclicker::get().update(logic.get_frame());

        if (Autoclicker::get().shouldPress()) {
            if (logic.autoclicker_player_1)
                gd::GameManager::sharedState()->getPlayLayer()->pushButton(0, false);
            if (logic.autoclicker_player_2 && play_layer->m_level->twoPlayerMode)
                gd::GameManager::sharedState()->getPlayLayer()->pushButton(0, true);
        }
        if (Autoclicker::get().shouldRelease()) {
            if (logic.autoclicker_player_1)
                gd::GameManager::sharedState()->getPlayLayer()->releaseButton(0, false);
            if (logic.autoclicker_player_2 && play_layer->m_level->twoPlayerMode)
                gd::GameManager::sharedState()->getPlayLayer()->releaseButton(0, true);
        }

        if (logic.autoclicker_auto_disable) {
            logic.autoclicker_disable_in--;
            if (logic.autoclicker_disable_in == 0) {
                logic.autoclicker_auto_disable = false;
                logic.autoclicker = false;
            }
        }
    }

    auto& audiospeedhack = AudiopitchHack::getInstance();
    bool isEnabled = audiospeedhack.isEnabled();
    if (isEnabled) {
        audiospeedhack.setPitch(self->getTimeScale());
    }
    else {
        audiospeedhack.setPitch(1);
    }

    if (logic.is_recording() || logic.is_playing()) {

        if (logic.recorder.m_recording) { // prevent screen tearing from lag in the first bits of lvl
            if ((logic.get_frame() / logic.fps) < 1.f || (1.f / dt) < logic.recorder.m_fps || !logic.recorder.real_time_rendering) {
                dt = 1.f / logic.fps;
                return CCScheduler_update(self, dt);
            }
        }
        const float target_dt = 1.f / logic.fps / logic.speedhack;

        if (logic.real_time_mode) {
            //mtx.lock();
            float speedhack = logic.speedhack;
            //mtx.unlock();

            g_disable_render = true;

            const unsigned int times = min(static_cast<int>((dt + g_left_over) / target_dt), 150);
            if (dt == 0.f) return CCScheduler_update(self, target_dt);

            for (unsigned i = 0; i < times; i++) {
                if (i == times - 1) {
                    g_disable_render = false;
                }
                CCScheduler_update(self, target_dt);
            }

            g_left_over += dt - target_dt * times;
            return;
        }
    }
    return CCScheduler_update(self, dt);
}

void __fastcall Hooks::CCKeyboardDispatcher_dispatchKeyboardMSG_h(CCKeyboardDispatcher* self, int, int key, bool down) {
    auto& logic = Logic::get();

    if (down) {
        auto advancing_key = Logic::get().keybinds.GetKeybind("advancing").key;
        if (advancing_key.has_value()) {
            if (ImGui::IsKeyPressed(advancing_key.value())) {
                Logic::get().start = std::chrono::steady_clock::now();
                Logic::get().frame_advance = false;
                bool old_real_time = Logic::get().real_time_mode;
                Logic::get().real_time_mode = false;			
                Hooks::CCScheduler_update_h(gd::GameManager::sharedState()->getScheduler(), 0, 1.f / logic.fps);
                Logic::get().frame_advance = true;
                Logic::get().real_time_mode = old_real_time;
            }
        }
    }
    else {
        Logic::get().start = std::chrono::steady_clock::time_point();
    }

    CCKeyboardDispatcher_dispatchKeyboardMSG(self, key, down);
}


bool __fastcall Hooks::PlayLayer::init_h(gd::PlayLayer* self, void* edx, gd::GJGameLevel* level) {
    auto& logic = Logic::get();
    logic.replay_index = 0;
    Clickbot::start = std::chrono::system_clock::now();

    bool ret = Hooks::PlayLayer::init(self, level);

    return ret;
}

void __fastcall Hooks::PlayLayer::updateVisibility_h(gd::PlayLayer* self) {
    if (!g_disable_render)
        updateVisibility(self);
}

bool __fastcall Hooks::PauseLayer::init_h(gd::PauseLayer* self) {
    auto& logic = Logic::get();

    if (logic.is_recording()) {
        auto& inputs = logic.get_inputs();
        auto last_input_p1 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return input.isPlayer2 == false; });
        auto last_input_p2 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return input.isPlayer2 == true; });

        if (last_input_p1 != inputs.rend() && last_input_p1->pressingDown && !logic.get_latest_checkpoint().player_1_data.m_isDashing) {
            logic.record_input(false, false);
        }

        if (last_input_p2 != inputs.rend() && last_input_p2->pressingDown && !logic.get_latest_checkpoint().player_2_data.m_isDashing) {
            logic.record_input(false, true);
        }
    }

    return Hooks::PauseLayer::init(self);
}

std::string formatTime(float timeInSeconds) {
    int hours = static_cast<int>(timeInSeconds / 60 / 60);
    int minutes = static_cast<int>(timeInSeconds / 60);
    int seconds = static_cast<int>(timeInSeconds) % 60;
    int milliseconds = static_cast<int>((timeInSeconds - static_cast<int>(timeInSeconds)) * 100);

    std::stringstream formattedTime;
    formattedTime << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;

    return formattedTime.str();
}

void __fastcall Hooks::PlayLayer::update_h(gd::PlayLayer* self, int, float dt) {
    auto& logic = Logic::get();

    static int offset = rand();

    if (self->m_hasCompletedLevel) {
        if (logic.is_recording()) logic.toggle_recording();
    }

    logic.player_acceleration = self->m_player1->m_xAccel;
    logic.player_speed = self->m_player1->m_playerSpeed;

    if (logic.recorder.m_recording)
        logic.recorder.handle_recording(self, dt);

    logic.previous_real_xpos = self->m_player1->getPositionX();

    if (!logic.player_x_positions.empty()) {
        if (!self->m_isDead && (self->m_player1->m_position.x != logic.player_x_positions.back() || self->m_hasCompletedLevel)) {
            if (self->m_hasCompletedLevel) {
                logic.calculated_xpos = self->m_player1->getPositionX();
                logic.calculated_frame = logic.get_frame();
            }
            else {

                logic.calculated_xpos = logic.xpos_calculation();
                logic.calculated_frame = round(logic.get_frame() + (self->m_player1->getPositionX() - logic.calculated_xpos));//round((logic.calculated_xpos / (logic.player_speed * logic.player_acceleration) * (1.f / logic.fps)) * logic.fps); // doesnt work when changing speeds, fucjk

                logic.previous_xpos = logic.calculated_xpos;
            }
        }
    }

    logic.player_x_positions.push_back(self->m_player1->m_position.x);

    if (logic.is_playing() && !logic.get_inputs().empty()) {
        if (logic.sequence_enabled) {
            if (logic.replay_index - 1 < logic.replays.size()) {
                Replay& selected_replay = logic.replays[logic.replay_index - 1];
                static int offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;

                if (logic.play_macro(offset)) {
                    offset = selected_replay.max_frame_offset > 0 ? (rand() % selected_replay.max_frame_offset / 2) - selected_replay.max_frame_offset : 0;
                }
            }
        }
        else {
            int _ = 0;
            logic.play_macro(_);
        }
    }

    logic.recorder.m_song_start_offset = self->m_levelSettings->m_songStartOffset;

    // Check if the frame counter label exists
    auto frame_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(FRAME_LABEL_ID);
    if (frame_counter) {
        if (logic.show_frame) {
            // Update the frame counter label with the current frame number
            char out[1000];
            int frame = logic.get_frame();
            std::string text = std::to_string(frame);
            int length = text.length();
            sprintf_s(out, "Frame: %i", frame);
            frame_counter->setAnchorPoint({ 0, 0.5 });
            frame_counter->setPosition(logic.frame_counter_x, logic.frame_counter_y); // Adjusted the x-position calculation
            frame_counter->setString(out);

        }
        else {
            // Remove and release the frame counter label if show_frame is false
            frame_counter->removeFromParent();
            frame_counter->release();
        }
    }
    else if (logic.show_frame) {
        auto frame_counter2 = cocos2d::CCLabelBMFont::create("Frame: ", "chatFont.fnt"); //probably leaks memory :p
        frame_counter2->setPosition(logic.frame_counter_x, logic.frame_counter_y);
        frame_counter2->setOpacity(70);

        self->addChild(frame_counter2, 999, FRAME_LABEL_ID);
    }

    auto recording_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(RECORDING_LABEL_ID);
    if (recording_label) {
        if (logic.is_recording()) {
            // Update the frame counter label with the current frame number
            char out[1000];
            sprintf_s(out, "Recording: %i", logic.inputs.size());
            recording_label->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.f, 20);
            recording_label->setString(out);
        }
        else {
            // Remove and release the frame counter label if show_frame is false
            recording_label->removeFromParent();
            recording_label->release();
        }
    }
    else if (logic.is_recording()) {
        auto recording_label2 = cocos2d::CCLabelBMFont::create("Recording: %i", "chatFont.fnt"); //probably leaks memory :p
        recording_label2->setPosition(cocos2d::CCDirector::sharedDirector()->getWinSize().width / 2.f, 20);
        recording_label2->setOpacity(70);

        self->addChild(recording_label2, 999, RECORDING_LABEL_ID);
    }

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    if (cps_counter) {
        if (logic.show_cps) {
            char out[24];
            sprintf_s(out, "CPS: %i", logic.count_presses_in_last_second());
            cps_counter->setString(out);
            if (logic.current_cps > logic.max_cps) {
                cps_counter->setColor({ 255, 0, 0 });
            }
            else if (logic.over_max_cps) {
                cps_counter->setColor({ 255, 72, 0 });
            }
            cps_counter->setAnchorPoint({ 0, 0.5 });
            cps_counter->setPosition(logic.cps_counter_x, logic.cps_counter_y);
        }

        else {
            cps_counter->removeFromParent();
            cps_counter->release();
        }
    }
    else if (logic.show_cps) {
        auto cps_counter2 = cocos2d::CCLabelBMFont::create("CPS: ", "chatFont.fnt");
        cps_counter2->setPosition(logic.cps_counter_x, logic.cps_counter_y);
        cps_counter2->setOpacity(70);

        self->addChild(cps_counter2, 999, CPS_LABEL_ID);
    }

    auto percent_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(PERCENT_LABEL_ID);;

    if (percent_label) {
        if (logic.show_percent) {
            char out[24];
            std::stringstream stream;
            float percent = min(100.f, (self->m_player1->getPositionX() / self->m_endPortal->getPositionX()) * 100.f);
            if (logic.percent_accuracy > 0) {
                stream << "%." << logic.percent_accuracy << "f%%";
                sprintf_s(out, stream.str().c_str(), percent);
            }
            else {
                stream << "%i%%";
                sprintf_s(out, stream.str().c_str(), static_cast<int>(percent));
            }
            percent_label->setString(out);
            percent_label->setAnchorPoint({ 0, 0.5 });
            percent_label->setPosition(logic.percent_counter_x, logic.percent_counter_y);
            percent_label->setScale(logic.percent_scale);
            percent_label->setOpacity(70);
        }

        else {
            percent_label->removeFromParent();
            percent_label->release();
        }
    }
    else if (logic.show_percent) {
        auto percent_label2 = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        percent_label2->setPosition(logic.percent_counter_x, logic.percent_counter_y);
        percent_label2->setScale(logic.percent_scale);
        percent_label2->setOpacity(70);

        self->addChild(percent_label2, 999, PERCENT_LABEL_ID);
    }

    auto time_label = (cocos2d::CCLabelBMFont*)self->getChildByTag(TIME_LABEL_ID);;
    if (time_label) {

        if (logic.show_time) {
            char out[24];
            const char* format = "%s";
            std::string str = formatTime(self->m_time);
            sprintf_s(out, str.c_str());
            time_label->setString(out);
            time_label->setAnchorPoint({ 0, 0.5 });
            time_label->setPosition(logic.time_counter_x, logic.time_counter_y);
            time_label->setScale(logic.time_scale);
            time_label->setOpacity(70);
        }

        else {
            time_label->removeFromParent();
            time_label->release();
        }
    }
    else if (logic.show_time) {
        auto time_label2 = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        time_label2->setPosition(logic.time_counter_x, logic.time_counter_y);
        time_label2->setScale(logic.time_scale);
        time_label2->setOpacity(70);

        self->addChild(time_label2, 999, TIME_LABEL_ID);
    }

    update(self, dt);
}


void play_sound(bool click, bool button) {
    auto& logic = Logic::get();

    if (!Clickbot::inited) {
        FMOD::System_Create(&Clickbot::system);
        Clickbot::system->init(1024 * 2, FMOD_INIT_NORMAL, nullptr);
        Clickbot::inited = true;
    }

    Clickbot::now = std::chrono::system_clock::now();
    Clickbot::cycleTime = Clickbot::now - Clickbot::start;

    std::string path = "";
    float volume = button ? logic.player_1_volume : logic.player_2_volume;
    bool softs = button ? logic.player_1_softs : logic.player_2_softs;
    bool hards = button ? logic.player_1_hards : logic.player_2_hards;
    std::string type = click ? "clicks" : "releases";
    auto channel = Clickbot::clickChannel;//click ? (button ? Clickbot::clickChannel : Clickbot::clickChannel2) : (button ? Clickbot::releaseChannel : Clickbot::releaseChannel2);
    auto sound = Clickbot::clickSound;//click ? (button ? Clickbot::clickSound : Clickbot::clickSound2) : (button ? Clickbot::releaseSound : Clickbot::releaseSound2);
    std::string folder = "";

    float softs_time = button ? logic.player_1_softs_time : logic.player_2_softs_time;
    float hards_time = button ? logic.player_1_hards_time : logic.player_2_hards_time;

    if (Clickbot::cycleTime.count() < (softs_time / 1000.f) && softs)
    {
        folder = "soft_" + type;
        volume = button ? logic.player_1_softs_volume : logic.player_2_softs_volume;
    }
    else if (Clickbot::cycleTime.count() > hards_time && hards) {
        folder = "hard_" + type;
        volume = button ? logic.player_1_hards_volume : logic.player_2_hards_volume;
    }
    else {
        folder = type;
    }

    path = Clickbot::pickRandomFile(folder, button);
    Clickbot::start = std::chrono::system_clock::now();

    if (!path.empty()) {
        Clickbot::system->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &sound);
        Clickbot::system->playSound(sound, nullptr, true, &channel);
        channel->setVolume((float)(volume));
        channel->setPaused(false);
    }

    Clickbot::system->update();
}

int __fastcall Hooks::PlayLayer::pushButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.clickbot_enabled) {
        play_sound(true, button);
    }

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            return 0;
        }
    }

    if (button && self->m_player1->m_isDashing) {
        return 0;
    }

    if (!button && self->m_player2->m_isDashing) {
        return 0;
    }

    if (logic.click_both_players && self->m_level->twoPlayerMode) {
        logic.record_input(true, !button);
        pushButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    if (logic.is_recording()) {
        if (button && !logic.record_player_1)
            return 0;
        if (!button && !logic.record_player_2)
            return 0;
    }

    logic.record_input(true, button);

    return pushButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::releaseButton_h(gd::PlayLayer* self, int, int idk, bool button) {
    auto& logic = Logic::get();

    if (logic.clickbot_enabled) {
        play_sound(false, button);
    }

    if (logic.is_playing()) {
        if (logic.ignore_actions_at_playback) {
            return 0;
        }
    }

    if (logic.click_both_players && self->m_level->twoPlayerMode) {
        logic.record_input(false, !button);
        releaseButton(self, idk, !button);
    }

    if (logic.swap_player_input) button = !button;

    if (logic.is_recording()) {
        if (button && !logic.record_player_1)
            return 0;
        if (!button && !logic.record_player_2)
            return 0;
    }

    logic.record_input(false, button);

    return releaseButton(self, idk, button);
}

int __fastcall Hooks::PlayLayer::resetLevel_h(gd::PlayLayer* self, int idk) {
    int ret = resetLevel(self); // calling the original function
    auto& logic = Logic::get();

    printf("\n");

    if (logic.is_recording()) logic.total_attempt_count++;

    logic.calculated_xpos = self->m_player1->getPositionX();
    logic.calculated_frame = round(logic.get_frame() + (self->m_player1->getPositionX() - logic.calculated_xpos));
    logic.previous_xpos = self->m_player1->getPositionX();
    logic.player_x_positions.clear();

    logic.player_x_positions.push_back(self->m_player1->getPositionX());

    //logic.calculated_xpos = logic.xpos_calculation();
    logic.previous_xpos = logic.xpos_calculation();

   /* if (logic.is_playing()) {
        releaseButton(self, 0, true);
        releaseButton(self, 0, false);
    }*/

    logic.live_inputs.clear();
    logic.over_max_cps = false;

    logic.end_portal_position = self->m_endPortal->getPositionX();

    auto cps_counter = (cocos2d::CCLabelBMFont*)self->getChildByTag(CPS_LABEL_ID);

    if (cps_counter) {
        cps_counter->setColor({ 255, 255, 255 });
    }

    // Section 1: Handle Sequencing
    if (logic.sequence_enabled) {
        if (!logic.is_recording() && (!logic.is_playing() || logic.sequence_enabled)) {
            Hooks::PlayLayer::releaseButton(self, 0, false);
            Hooks::PlayLayer::releaseButton(self, 0, true);
            if (logic.replay_index < logic.replays.size() && logic.sequence_enabled) {
                logic.get_inputs() = logic.replays[logic.replay_index].actions;
            }
            else {
                logic.replay_index = 0;
                logic.get_inputs().clear();
            }
        }
        logic.replay_index += 1;
    }

    // INITIALIZES MACRO NAME
    if (self->m_currentAttempt == 1) {
        strcpy(logic.macro_name, self->m_level->levelName.c_str());
        logic.recorder.video_name = logic.macro_name;
    }

    if (logic.is_playing()) {
        logic.set_replay_pos(logic.find_closest_input());
    }

    if (!self->m_isPracticeMode)
        logic.checkpoints.clear();

    if (logic.is_recording() || logic.is_playing())
        logic.handle_checkpoint_data();

    // Section 3: Update Time and Activated Objects
    if (self->m_checkpoints->count() > 0) {
        logic.calculated_xpos = logic.checkpoints.back().calculated_xpos;

        self->m_time = logic.get_latest_offset();
        constexpr auto delete_from = [&](auto& vec, size_t index) {
            vec.erase(vec.begin() + index, vec.end());
        };

        delete_from(logic.activated_objects, logic.checkpoints.back().activated_objects_size);
        delete_from(logic.activated_objects_p2, logic.checkpoints.back().activated_objects_p2_size);

        if (logic.is_recording()) {
            for (const auto& object : logic.activated_objects)
                object->m_hasBeenActivated = true;
            for (const auto& object : logic.activated_objects_p2)
                object->m_hasBeenActivatedP2 = true;
        }
    }
    else {
        logic.activated_objects.clear();
        logic.activated_objects_p2.clear();
    }

    logic.recorder.update_song_offset(self);

    // Section 4: Handle Recording
    if (logic.is_recording()) {

        logic.cps_over_percents.clear();
        if (logic.get_inputs().empty()) return ret;

        if (!logic.checkpoints.empty()) {
            logic.set_removed(logic.get_removed() + (logic.get_frame() - logic.get_latest_checkpoint().number));

            if (logic.record_player_1)
                logic.remove_inputs(logic.get_frame(), true);
            if (logic.record_player_2)
                logic.remove_inputs(logic.get_frame(), false);

            printf("%i", logic.get_frame());

            if (!logic.get_inputs().empty()) {
                auto& inputs = logic.get_inputs();

                auto last_input_p1 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return !input.isPlayer2; });
                auto last_input_p2 = std::find_if(inputs.rbegin(), inputs.rend(), [](Frame input) { return input.isPlayer2; });

                bool currently_holdingP1 = self->m_player1->m_isHolding;
                bool currently_holdingP2 = self->m_player2->m_isHolding;

                if (last_input_p1 != inputs.rend()) {
                    if (last_input_p1->pressingDown && logic.record_player_1 && !currently_holdingP1 && !logic.get_latest_checkpoint().player_1_data.m_isDashing) {
                        logic.add_input({ logic.get_latest_checkpoint().number, false, false });
                    }
                    else if (logic.record_player_1 && currently_holdingP1 && !logic.get_latest_checkpoint().player_1_data.m_isDashing) {
                        logic.add_input({ logic.get_latest_checkpoint().number, true, false });
                    }
                }

                if (last_input_p2 != inputs.rend()) {
                    if (last_input_p2->pressingDown && logic.record_player_2 && !currently_holdingP2 && self->m_level->twoPlayerMode && (!logic.get_latest_checkpoint().player_1_data.m_isDashing || !logic.get_latest_checkpoint().player_2_data.m_isDashing)) {
                        logic.add_input({ logic.get_latest_checkpoint().number, false, true });
                    }
                    else if (logic.record_player_2 && currently_holdingP2 && self->m_level->twoPlayerMode && (!logic.get_latest_checkpoint().player_1_data.m_isDashing || !logic.get_latest_checkpoint().player_2_data.m_isDashing)) {
                        logic.add_input({ logic.get_latest_checkpoint().number, true, true });
                    }
                }
            }
        }
        else {
            if (logic.record_player_1)
                logic.remove_inputs(logic.get_frame(), true);
            if (logic.record_player_2)
                logic.remove_inputs(logic.get_frame(), false);

            logic.set_removed(0);
            self->m_time = 0;
        }

        if (logic.record_player_1 && self->m_player1->m_isHolding && logic.checkpoints.empty())
            logic.record_input(true, true);
        if (logic.record_player_2 && self->m_player2->m_isHolding && logic.checkpoints.empty())
            logic.record_input(true, false);
    }

    return ret;
}

void __fastcall Hooks::PlayerObject_ringJump_h(gd::PlayerObject* self, int, gd::GameObject* ring) {
    PlayerObject_ringJump(self, ring);
    auto& logic = Logic::get();
    if (logic.is_recording() && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(ring) + 0x2ca)) {
        logic.activated_objects.push_back(ring);
    }
}

void __fastcall Hooks::GameObject_activateObject_h(gd::GameObject* self, int, gd::PlayerObject* player) {
    GameObject_activateObject(self, player);
    auto& logic = Logic::get();
    if (logic.is_recording() && *reinterpret_cast<bool*>(reinterpret_cast<uintptr_t>(self) + 0x2ca)) {
        logic.activated_objects.push_back(self);
    }
}

void* __fastcall Hooks::PlayLayer::exitLevel_h(gd::PlayLayer* self, int) {
    auto& logic = Logic::get();

    logic.autoclicker = false;
    logic.checkpoints.clear();
    logic.set_removed(0);

    return exitLevel(self);
}

int __fastcall Hooks::createCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.add_offset(self->m_time);
    CheckpointData checkpointData1 = CheckpointData::create(self->m_player1);
    CheckpointData checkpointData2 = CheckpointData::create(self->m_player2);

    logic.save_checkpoint({ logic.get_frame(), checkpointData1, checkpointData2, logic.activated_objects.size(), logic.activated_objects_p2.size(), logic.calculated_xpos });

    return createCheckpoint(self);
}

int __fastcall Hooks::removeCheckpoint_h(gd::PlayLayer* self) {
    auto& logic = Logic::get();

    logic.remove_last_offset();
    logic.remove_last_checkpoint();

    return removeCheckpoint(self);
}