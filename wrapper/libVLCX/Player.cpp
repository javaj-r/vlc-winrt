/*****************************************************************************
 * Copyright © 2013 VideoLAN
 *
 * Authors: Kellen Sunderland
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "pch.h"
#include "Player.h"
#include <map>

using namespace libVLCX;
using namespace Windows::Graphics::Display;

char *
FromPlatformString(Platform::String^ str) {
    size_t len = WideCharToMultiByte(CP_UTF8, 0, str->Data(), -1, NULL, 0, NULL, NULL);
    if(len == 0)
        return NULL;
    char* psz_str = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, str->Data(), -1, psz_str, len, NULL, NULL);
    return psz_str;
}

Platform::String^
ToPlatformString(const char *str) {
    size_t len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    if(len == 0)
        return nullptr;
    wchar_t* w_str = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, w_str, len);
    return ref new Platform::String(w_str);
}

size_t Player::ToCharArray(Platform::String^ str, char *arr, size_t maxSize)
{
    size_t nbConverted = 0;
    wcstombs_s(&nbConverted, arr, 128, str->Data(), maxSize);
    return nbConverted;
}


Player::Player(SwapChainBackgroundPanel^ panel) :p_mp(NULL), p_instance(NULL)
{
    OutputDebugStringW(L"Hello, Player!");
    p_panel = panel;
    p_dxManager = new DirectXManger();

    m_displayWidth = (float)(p_panel->ActualWidth * (float)DisplayProperties::ResolutionScale / 100.0f);
    m_displayHeight = (float)(p_panel->ActualHeight * (float)DisplayProperties::ResolutionScale / 100.0f);
}

//Todo: don't block UI during initialization
IAsyncAction^ Player::Initialize()
{
    p_dxManager->CreateSwapPanel(p_panel);

    IAsyncAction^ vlcInitTask = ThreadPool::RunAsync(ref new WorkItemHandler([=](IAsyncAction^ operation)
    {
        this->InitializeVLC();
    }, Platform::CallbackContext::Any), WorkItemPriority::High, WorkItemOptions::TimeSliced);
    return vlcInitTask;
}

void Player::InitializeVLC()
{
    char ptr_d2dstring[40];
    sprintf_s(ptr_d2dstring, "--winrt-d2dcontext=0x%p", p_dxManager->cp_d2dContext);

    char ptr_scstring[40];
    sprintf_s(ptr_scstring, "--winrt-swapchain=0x%p", p_dxManager->cp_swapChain);

    char widthstring[40];
    sprintf_s(widthstring, "--winrt-width=%f", m_displayWidth);

    char heightstring[40];
    sprintf_s(heightstring, "--winrt-height=%f", m_displayHeight);

    char fontstring[128 + 28];
    char packagePath[128];

    Windows::ApplicationModel::Package^ currentPackage = Windows::ApplicationModel::Package::Current;
    ToCharArray(currentPackage->InstalledLocation->Path, packagePath, 128);
    sprintf_s(fontstring, "--freetype-font=%s\\segoeui.ttf", packagePath);

    /* Don't add any invalid options, otherwise it causes LibVLC to fail */
    const char *argv[] = {
        "-I",
        "dummy",
        "--no-osd",
        "--verbose=2",
        "--no-stats",
        ptr_d2dstring,
        ptr_scstring,
        widthstring,
        heightstring,
        "--avcodec-fast",
        "--no-avcodec-dr",
        fontstring
    };

    p_instance = libvlc_new(sizeof(argv) / sizeof(*argv), argv);
    if (!p_instance) {
        throw new std::exception("Could not initialise libvlc!", 1);
        return;
    }
}

void Player::UpdateSize(unsigned int x, unsigned int y)
{
    // Have to update the size there
}

void Player::MediaEndedCall(){
    MediaEnded();
}

void vlc_event_callback(const libvlc_event_t *ev, void *data)
{
    Player ^player = ((PlayerPointerWrapper*)data)->player;
    if (ev->type == libvlc_MediaPlayerEndReached)
    {
        player->DetachEvent();
        player->MediaEndedCall();
    }
}

void Player::DetachEvent(){
    libvlc_event_manager_t *ev = libvlc_media_player_event_manager(p_mp);
    static const libvlc_event_type_t mp_events[] = {
        libvlc_MediaPlayerPlaying,
        libvlc_MediaPlayerPaused,
        libvlc_MediaPlayerEndReached,
        libvlc_MediaPlayerStopped,
        libvlc_MediaPlayerVout,
        libvlc_MediaPlayerPositionChanged,
        libvlc_MediaPlayerEncounteredError
    };
    for (int i = 0; i < (sizeof(mp_events) / sizeof(*mp_events)); i++)
        libvlc_event_detach(ev, mp_events[i], vlc_event_callback, new PlayerPointerWrapper(this));
}

void Player::Open(Platform::String^ mrl)
{
    const char *p_mrl = FromPlatformString(mrl);
    if (p_instance){
        libvlc_media_t* m = libvlc_media_new_location(this->p_instance, p_mrl);
        p_mp = libvlc_media_player_new_from_media(m);

        /* Connect the event manager */
        libvlc_event_manager_t *ev = libvlc_media_player_event_manager(p_mp);
        static const libvlc_event_type_t mp_events[] = {
            libvlc_MediaPlayerPlaying,
            libvlc_MediaPlayerPaused,
            libvlc_MediaPlayerEndReached,
            libvlc_MediaPlayerStopped,
            libvlc_MediaPlayerVout,
            libvlc_MediaPlayerPositionChanged,
            libvlc_MediaPlayerEncounteredError
        };

        for (int i = 0; i < (sizeof(mp_events) / sizeof(*mp_events)); i++)
        {
            libvlc_event_attach(ev, mp_events[i], vlc_event_callback, new PlayerPointerWrapper(this));
        }

        libvlc_media_release(m);
    }

    delete[](p_mrl);
}

void Player::Stop()
{
    if (p_mp)
    {
        libvlc_media_player_stop(p_mp);
    }
    return;
}

void Player::Pause()
{
    if (p_mp)
    {
        libvlc_media_player_pause(p_mp);
    }
    return;
}

void Player::Play()
{
    if (p_mp)
    {
        libvlc_media_player_play(p_mp);
    }
    return;
}

void Player::Seek(float position)
{
    if (p_mp)
    {
        if (libvlc_media_player_is_seekable(p_mp))
        {
            libvlc_media_player_set_position(p_mp, position);
        }
    }
}

void Debug( const wchar_t *fmt, ...) {
    wchar_t buf[255];
    va_list args;
    va_start(args, fmt);
    vswprintf_s(buf, fmt, args);
    va_end(args);
    OutputDebugStringW(buf);
}

float Player::GetPosition()
{
    float position = 0.0f;
    if (p_mp)
    {
        position = libvlc_media_player_get_position(p_mp);
    }

    int i = GetAudioTracksCount();
    int j = GetSubtitleCount();
    Debug( L"Audio Tracks: %i %i\n", i, j );

    return position;
}

int64 Player::GetLength()
{
    int64 length = 0;
    if (p_mp)
    {
        length = libvlc_media_player_get_length(p_mp);
    }
    return length;
}

int64 Player::GetTime()
{
    int64 time = 0;
    if (p_mp)
    {
        time = libvlc_media_player_get_time(p_mp);
    }
    return time;
}

int Player::GetSubtitleCount(){
    int subtitleTrackCount = 0;
    if (p_mp)
    {
        subtitleTrackCount = libvlc_video_get_spu_count(p_mp);
    }
    return subtitleTrackCount;
}

int Player::GetSubtitleDescription(Collections::IMap<int,Platform::String ^> ^tracks) {
    libvlc_track_description_t *subtitleTrackDesc = NULL;
    int count = 0;
    if (p_mp && tracks) {
        subtitleTrackDesc = libvlc_video_get_spu_description(p_mp);
        while (subtitleTrackDesc != NULL ) {
			tracks->Insert(subtitleTrackDesc->i_id, ToPlatformString(subtitleTrackDesc->psz_name));
            subtitleTrackDesc = subtitleTrackDesc->p_next;
            count++;
        }
    }
    libvlc_track_description_list_release(subtitleTrackDesc);
    return count;
}

int Player::SetSubtitleTrack(int track){
    int spuLoaded = 0;
    if (p_mp)
    {
        spuLoaded = libvlc_video_set_spu(p_mp, track);
    }
    return spuLoaded;
}

int Player::GetAudioTracksCount(){
    int audioTracksCount = 0;
    if (p_mp)
    {
        audioTracksCount = libvlc_audio_get_track_count (p_mp);
    }
    return audioTracksCount;
}

int Player::GetAudioTracksDescription(Collections::IMap<int,Platform::String ^> ^tracks) {
    libvlc_track_description_t *audioTrackDesc = NULL;
    int count = 0;
    if (p_mp) {
        audioTrackDesc = libvlc_audio_get_track_description(p_mp);
        while (audioTrackDesc != NULL && audioTrackDesc->p_next != NULL) {
            tracks->Insert(audioTrackDesc->i_id, ToPlatformString(audioTrackDesc->psz_name));
            audioTrackDesc = audioTrackDesc->p_next;
            count++;
        }
    }
    libvlc_track_description_list_release(audioTrackDesc);
    return count;
}

int Player::SetAudioTrack(int track){
    int audioTrackLoaded = 0;
    if (p_mp)
    {
        audioTrackLoaded = libvlc_audio_set_track(p_mp, track);
    }
    return audioTrackLoaded;
}

Player::~Player()
{
    if (p_mp){
        libvlc_media_player_release(p_mp);
    }
    if (p_instance){
        libvlc_release(p_instance);
    }


    /*if (p_dxManager){
        delete p_dxManager;
        p_dxManager = nullptr;
        }*/
}


