﻿#include "HostSettingsWidget.h"

HostSettingsWidget::HostSettingsWidget(Hosting& hosting, function<void(bool)> onHostRunningStatusCallback)
    : _hosting(hosting), _audioIn(_hosting.audioIn), _audioOut(_hosting.audioOut),
    _thumbnails(_hosting.getSession().getThumbnails()), _onHostRunningStatusCallback(onHostRunningStatusCallback)
{
    ParsecHostConfig cfg = hosting.getHostConfig();
    try
    {
        strcpy_s(_roomName, cfg.name);
        strcpy_s(_gameID, cfg.gameID);
        strcpy_s(_secret, cfg.secret);
        strcpy_s(_kioskApplication, "");
        strcpy_s(_kioskParam, "");
    }
    catch (const std::exception&)
    {
        try
        {
            strcpy_s(_roomName, "");
            strcpy_s(_gameID, "");
            strcpy_s(_secret, "");
            strcpy_s(_kioskApplication, "");
            strcpy_s(_kioskParam, "");
        } catch (const std::exception&) {}
    }
    _publicGame = cfg.publicGame;
    _maxGuests = cfg.maxGuests;
    
    _micVolume = MetadataCache::preferences.micVolume;
    _audioIn.volume = (float)_micVolume / 100.0f;
    
    _speakersVolume = MetadataCache::preferences.speakersVolume;
    _audioOut.volume = (float)_speakersVolume / 100.0f;

    _audioIn.isEnabled = MetadataCache::preferences.micEnabled;
    _audioOut.isEnabled = MetadataCache::preferences.speakersEnabled;

    _latencyLimiter = MetadataCache::preferences.latencyLimitEnabled;
    _latencyLimit = MetadataCache::preferences.latencyLimitValue;

    _hotseat = MetadataCache::preferences.hotseat;
    _hotseatTime = MetadataCache::preferences.hotseatTime;

    _kioskMode = MetadataCache::preferences.kioskMode;

    vector<Thumbnail>::iterator it;
    for (it = _thumbnails.begin(); it != _thumbnails.end(); ++it)
    {
        if ((*it).gameId.compare(_gameID) == 0)
        {
            _thumbnailName = (*it).name;
        }
    }

    if (strlen(_secret) == 0) {
        try { strcpy_s(_secret, "play-now"); }
        catch (const std::exception&) {}
    }
    updateSecretLink();

    if (strlen(_kioskApplication) == 0) {
        try { strcpy_s(_kioskApplication, MetadataCache::preferences.kioskApplication.c_str()); }
        catch (const std::exception&) {}
    }

    if (strlen(_kioskParam) == 0) {
        try { strcpy_s(_kioskParam, MetadataCache::preferences.kioskParameters.c_str()); }
        catch (const std::exception&) {}
    }
}

bool HostSettingsWidget::render(HWND& hwnd)
{
    static float indentSize = 0;
    static ImVec2 dummySize = ImVec2(0.0f, 3.0f);
    static ImVec2 cursor;

    AppStyle::pushTitle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(250, 250), ImVec2(600, 900));
    ImGui::Begin("Host Settings", (bool*)0);
    AppStyle::pushLabel();

    static ImVec2 size;
    static ImVec2 pos;
    size = ImGui::GetContentRegionAvail();
    pos = ImGui::GetWindowPos();

    //if (_hosting.isRunning() && isDirty())
    //{
    //    cursor = ImGui::GetCursorPos();
    //    ImGui::SetCursorPos(ImVec2(size.x - 30.0f, 35.0f));

    //    if (IconButton::render(AppIcons::submit, AppColors::primary))
    //    {
    //        _hosting.setHostConfig(_roomName, _gameID, _maxGuests, _publicGame, _secret);
    //        _hosting.applyHostConfig();
    //        savePreferences();
    //    }
    //    TitleTooltipWidget::render("Update Room Settings", "The room will be instantly updated with your new settings.");

    //    ImGui::SetCursorPos(cursor);
    //}
    //
    //ImGui::Dummy(ImVec2(0, 10.0f));

    ImGui::Text("ROOM NAME");
    if (strlen(_roomName) <= 28)
        AppStyle::pushPositive();
    else
        AppStyle::pushInput();
    ImGui::SetNextItemWidth(size.x);
    //AppStyle::pushInput();
    ImGui::InputText(" ", _roomName, 44);
    //ImGui::Text("Room name");
    //AppStyle::pushInput();
    //ImGui::InputTextMultiline(" ", _roomName, HOST_NAME_LEN, ImVec2(size.x, 50));
    TitleTooltipWidget::render("Room Title", "The text displayed below thumbnails in Arcade list.");
    AppStyle::pop();

    ImGui::Dummy(dummySize);

    ImGui::Text("THUMBNAIL");
    ImGui::SetNextItemWidth(size.x);
    AppStyle::pushInput();
    if (ImGui::BeginCombo("### Thumbnail picker combo", _thumbnailName.c_str(), ImGuiComboFlags_HeightLarge))
    {
        for (size_t i = 0; i < _thumbnails.size(); ++i)
        {
            bool isSelected = (_thumbnails[i].gameId.compare(_gameID) == 0);
            if (ImGui::Selectable(_thumbnails[i].name.c_str(), isSelected))
            {
                _thumbnailName = _thumbnails[i].name;
                try
                {
                    strcpy_s(_gameID, GAME_ID_LEN, _thumbnails[i].gameId.c_str());
                }
                catch (const std::exception&) {}
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    TitleTooltipWidget::render("Thumbnail Picker", "To find new thumbnails, go to the Arcade Thumbnails window.");
    AppStyle::pop();

    ImGui::Dummy(dummySize);

    ImGui::Text("ROOM SECRET");
    ImGui::SetNextItemWidth(size.x);
    AppStyle::pushInput();
    if (strlen(_secret) < LINK_COMPATIBLE_SECRET_SIZE - 1) AppColors::pushNegative();
    else AppColors::pushPositive();
    if (ImGui::InputText("##Secret input", _secret, LINK_COMPATIBLE_SECRET_SIZE))
    {
        updateSecretLink();
    }
    AppColors::pop();
    TitleTooltipWidget::render("Room Secret", "Generates the share link that lets people\njoin your room anytime.\nMust have 8 characters.");
    AppStyle::pop();

    ImGui::Dummy(dummySize);
    
    ImGui::Text("SHARE LINK");
    ImGui::SetNextItemWidth(size.x);
    AppStyle::pushInput();
    ImGui::InputText("##Secret link", _secretLink, 128, ImGuiInputTextFlags_ReadOnly);

    ImGui::Dummy(dummySize);

    cursor = ImGui::GetCursorPos();

    AppStyle::pushLabel();
    ImGui::Text("KIOSK MODE");
    ImGui::BeginChild("##Kiosk mode child", ImVec2(size.x, 60.0f));
        if (ToggleIconButtonWidget::render(AppIcons::yes, AppIcons::no, _kioskMode, AppColors::positive, AppColors::negative, ImVec2(22, 22))) {
            MetadataCache::preferences.kioskMode = !MetadataCache::preferences.kioskMode;
            _kioskMode = MetadataCache::preferences.kioskMode;
        }
        if (MetadataCache::preferences.kioskMode)    TitleTooltipWidget::render("Kiosk Mode Off", "Don't launch in kiosk mode.");
        else                    TitleTooltipWidget::render("Kiosk Mode On", "Automatically restart a program if it is closed.");

        ImGui::SameLine();
        ImGui::SetNextItemWidth(size.x - 35);
        AppStyle::pushInput();
        if (ImGui::InputText("##Kioskapplication", _kioskApplication, 256)) {
            MetadataCache::preferences.kioskApplication = _kioskApplication;
        }
        TitleTooltipWidget::render("Kiosk Application File Path", "Set the file path of the program you wish to restart.");

        ImGui::SetNextItemWidth(size.x);
        if (ImGui::InputText("##Kioskparameters", _kioskParam, 256)) {
            MetadataCache::preferences.kioskParameters = _kioskParam;
        }
        TitleTooltipWidget::render("Kiosk Parameters", "Pass any additional parameters to the kiosk application.");
    ImGui::EndChild();

    AppStyle::pushLabel();

    ImGui::BeginChild("##Guest slot child", ImVec2(size.x/3, 50.0f));
        ImGui::Text("GUEST SLOTS");
        if (IntRangeWidget::render("guest count", _maxGuests, 0, 64, 0.025f)) {
            TitleTooltipWidget::render("Room Slots", "How many guests do you want in this room?");
        }
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##Public room child", ImVec2(size.x / 3, 50.0f));
        ImGui::Text("PUBLIC ROOM");
        ImGui::Indent(20);
        if (ToggleIconButtonWidget::render(AppIcons::yes, AppIcons::no, _publicGame, AppColors::positive, AppColors::negative, ImVec2(22, 22))) {
            _publicGame = !_publicGame;
        }
        if (_publicGame)    TitleTooltipWidget::render("Public Game", "Anyone can enter this room.");
        else                TitleTooltipWidget::render("Private Game", "All guests must use the secret link to enter this room.");
    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("##Latency child", ImVec2(size.x / 3, 50.0f));
    ImGui::Text("LATENCY LIMITER");
        if (ToggleIconButtonWidget::render(AppIcons::yes, AppIcons::no, _latencyLimiter, AppColors::positive, AppColors::negative, ImVec2(22, 22))) {
            _latencyLimiter = !_latencyLimiter;
            MetadataCache::preferences.latencyLimitEnabled = _latencyLimiter;
        }
        if (_latencyLimiter)    TitleTooltipWidget::render("Latency Limit Off", "Anyone can join.");
        else                    TitleTooltipWidget::render("Latency Limit On", "Guests who's pings exceed the value on the right will be automatically kicked.");

        ImGui::SameLine();

        if (IntRangeWidget::render("latency limit", _latencyLimit, 0, 64, 0.025f)) {
            TitleTooltipWidget::render("Latency Limit", "The max ping allowed.");
            MetadataCache::preferences.latencyLimitValue = _latencyLimit;
        }
    ImGui::EndChild();

    ImGui::Dummy(dummySize);

    ImGui::BeginChild("##Hotseat child", ImVec2(120.0f, 50.0f));
    ImGui::Text("AUTOMATIC HOTSEAT");
    if (ToggleIconButtonWidget::render(AppIcons::yes, AppIcons::no, _hotseat, AppColors::positive, AppColors::negative, ImVec2(22, 22))) {
        _hotseat = !_hotseat;
        MetadataCache::preferences.hotseat = _hotseat;
    }
    if (_hotseat)           TitleTooltipWidget::render("Hotseat Off", "Players won't be swapped.");
    else                    TitleTooltipWidget::render("Hotseat On", "Guests will automatically swapped in gamepad 1 after set minutes.");

    ImGui::SameLine();

    if (IntRangeWidget::render("hotseat time", _hotseatTime, 0, 60, 0.025f)) {
        TitleTooltipWidget::render("Hotseat Time", "Time before player 1 is swapped (in minutes).");
        MetadataCache::preferences.hotseatTime = _hotseatTime;
    }
    ImGui::EndChild();

    static bool showPopup = false;
    static string popupTitle = "";
    popupTitle = (_hosting.isRunning() ? "Stop hosting?" : "Start hosting?");

    indentSize = 0.5f * size.x - 60.0f;
    ImGui::Indent(indentSize);
    if (ToggleIconButtonWidget::render(
        AppIcons::stop, AppIcons::play, _hosting.isRunning(),
        AppColors::negative, AppColors::positive, ImVec2(100, 100)
    ))
    {
        showPopup = true;
        ImGui::OpenPopup(popupTitle.c_str());
        
        savePreferences();
    }
    ImGui::Unindent(indentSize);

    if (_hosting.isRunning())   TitleTooltipWidget::render("Stop hosting", "Close current room.");
    else                        TitleTooltipWidget::render("Start hosting", "Open a new room using these settings.");

    // ================================================================================

    if (ConfirmPopupWidget::render(popupTitle.c_str(), showPopup))
    {
        // Was clicked and is already running (must stop)
        if (_hosting.isRunning())
        {
            _hosting.stopHosting();
            if (_onHostRunningStatusCallback != nullptr) _onHostRunningStatusCallback(false);
        }

        // Was clicked and is not running (must start)
        else
        {
            _hosting.setHostConfig(_roomName, _gameID, _maxGuests, _publicGame, _secret);
            _hosting.applyHostConfig();
            _hosting.startHosting();
            if (_onHostRunningStatusCallback != nullptr) _onHostRunningStatusCallback(true);
        }
    }

    if (_hosting.isRunning() && isDirty())
    {
        //cursor = ImGui::GetCursorPos();
        //ImGui::SetCursorPos(ImVec2(size.x - 30.0f, 25.0f));
        ImGui::SameLine();

        if (IconButton::render(AppIcons::submit, AppColors::primary, ImVec2(30, 30)))
        {
            _hosting.setHostConfig(_roomName, _gameID, _maxGuests, _publicGame, _secret);
            _hosting.applyHostConfig();
            savePreferences();
        }
        TitleTooltipWidget::render("Update Room Settings", "The room will be instantly updated with your new settings.");

        //ImGui::SetCursorPos(cursor);
    }

    // ================================================================================

    ImGui::Dummy(dummySize);

    if (!_hosting.isRunning() && _hosting.isReady())
    {
        if (!_hosting._disableMicrophone) _audioIn.captureAudio();
        _audioOut.captureAudio();
    }

    static int previousMicVolume, previousSpeakersVolume;
    static bool isVolumeChanged = false;
    static float targetPreview;

    if (!_hosting._disableMicrophone)
    {

        previousMicVolume = _micVolume;
        previousSpeakersVolume = _speakersVolume;

        static float micPreview;
        _micVolume = (int)(100.0f * _audioIn.volume);
        targetPreview = AudioTools::decibelToFloat(_audioIn.popPreviewDecibel());
        micPreview = lerp(micPreview, targetPreview, easing(targetPreview - micPreview));
        if (AudioControlWidget::render("Microphone", &_micVolume, _audioIn.isEnabled, micPreview, AppIcons::micOn, AppIcons::micOff))
        {
            _audioIn.isEnabled = !_audioIn.isEnabled;
            savePreferences();
        }
        _audioIn.volume = (float)_micVolume / 100.0f;

    }

    static float speakersPreview;
    _speakersVolume = (int)(100.0f *_audioOut.volume);
    targetPreview = AudioTools::decibelToFloat(_audioOut.popPreviewDecibel());
    speakersPreview = lerp(speakersPreview, targetPreview, easing(targetPreview - speakersPreview));
    if (AudioControlWidget::render("Speakers", &_speakersVolume, _audioOut.isEnabled, speakersPreview, AppIcons::speakersOn, AppIcons::speakersOff))
    {
        _audioOut.isEnabled = !_audioOut.isEnabled;
        savePreferences();
    }
    _audioOut.volume = (float)_speakersVolume / 100.0f;

    static Debouncer debouncer(DEBOUNCE_TIME_MS, [&]() { savePreferences(); });
    if (_micVolume != previousMicVolume || _speakersVolume != previousSpeakersVolume)
    {
        debouncer.start();
    }

    AppStyle::pop();
    ImGui::End();
    AppStyle::pop();

    return true;
}

void HostSettingsWidget::savePreferences()
{
    MetadataCache::preferences.roomName = _roomName;
    MetadataCache::preferences.gameID = strlen(_gameID) <= 25 ? "1wdoHfhhZH5lPuZCwGBete0HIAj" : _gameID;
    MetadataCache::preferences.guestCount = _maxGuests;
    MetadataCache::preferences.publicRoom = _publicGame;
    MetadataCache::preferences.secret = _secret;
    MetadataCache::preferences.micVolume = _micVolume;
    MetadataCache::preferences.micEnabled = _audioIn.isEnabled;
    MetadataCache::preferences.speakersVolume = _speakersVolume;
    MetadataCache::preferences.speakersEnabled = _audioOut.isEnabled;
    MetadataCache::preferences.latencyLimitEnabled = _latencyLimiter;
    MetadataCache::preferences.latencyLimitValue = _latencyLimit;
    MetadataCache::preferences.kioskApplication = _kioskApplication;
    MetadataCache::preferences.kioskParameters = _kioskParam;
    MetadataCache::savePreferences();
}

void HostSettingsWidget::updateSecretLink()
{
    try
    {
        strcpy_s(
            _secretLink,
            128,
            (string("https://parsec.gg/g/") + _hosting.getSession().hostPeerId + "/" + _secret + "/").c_str()
        );
    }
    catch (const std::exception&) {}
}

bool HostSettingsWidget::isDirty()
{
    ParsecHostConfig cfg = _hosting.getHostConfig();

    if (
        _publicGame != cfg.publicGame ||
        _maxGuests != cfg.maxGuests ||
        (strcmp(_roomName, cfg.name) != 0) ||
        (strcmp(_gameID, cfg.gameID) != 0) ||
        (strcmp(_secret, cfg.secret) != 0)
    ) return true;

    return false;
}

float HostSettingsWidget::lerp(float val1, float val2, float t)
{
    return (1-t) * val1 + t * val2;
}

float HostSettingsWidget::easing(float t)
{
    return 0.3f * max(abs(t), 0.4f);
}
