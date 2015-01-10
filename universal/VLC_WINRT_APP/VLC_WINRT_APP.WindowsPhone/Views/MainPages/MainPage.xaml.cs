﻿using System.Diagnostics;
using Windows.Graphics.Display;
using Windows.Media;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using VLC_WINRT_APP.Helpers;
using VLC_WINRT_APP.Services.Interface;
using VLC_WINRT_APP.Services.RunTime;
using System;

namespace VLC_WINRT_APP.Views.MainPages
{
    /// <summary>
    /// Une page vide peut être utilisée seule ou constituer une page de destination au sein d'un frame.
    /// </summary>
    public sealed partial class MainPage : SwapChainPanel
    {
        private readonly IMediaService _mediaService;
        public MainPage(IMediaService mediaService)
        {
            this.InitializeComponent();
            _mediaService = mediaService;
            (mediaService as MediaService).SetMediaElement(MediaElement);
            Loaded += SwapPanelLoaded;
            ApplicationView.GetForCurrentView().SetDesiredBoundsMode(ApplicationViewBoundsMode.UseCoreWindow);
            DisplayInformation.GetForCurrentView().OrientationChanged += DisplayPropertiesOnOrientationChanged;
        }

        private void OnSizeChanged(object sender, SizeChangedEventArgs sizeChangedEventArgs)
        {
            _mediaService.SetSizeVideoPlayer((uint)sizeChangedEventArgs.NewSize.Width, (uint)sizeChangedEventArgs.NewSize.Height);
        }

        private async void DisplayPropertiesOnOrientationChanged(DisplayInformation info, object sender)
        {
            StatusBar sb = StatusBar.GetForCurrentView();

            var appView = ApplicationView.GetForCurrentView();

            if (DisplayHelper.IsPortrait())
            {
                await sb.ShowAsync();
                appView.SuppressSystemOverlays = false;
            }
            else
            {
                await sb.HideAsync();
                appView.SuppressSystemOverlays = true;
            }
        }

        private void SwapPanelLoaded(object sender, RoutedEventArgs e)
        {
            _mediaService.Initialize(SwapChainPanel);
            SizeChanged += OnSizeChanged;
            Unloaded += MainPage_Unloaded;
        }

        private void MainPage_Unloaded(object sender, RoutedEventArgs e)
        {
            SizeChanged -= OnSizeChanged;
        }

        private void CommandBar_OnHomeButtonClicked(Button button, EventArgs e)
        {
            if (App.ApplicationFrame.CurrentSourcePageType != typeof(MainPageHome))
                App.ApplicationFrame.Navigate(typeof(MainPageHome));
        }
    }
}
