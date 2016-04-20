﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Windows.Storage;
using Windows.Storage.FileProperties;
using Windows.Storage.Search;
using Windows.UI.Core;
using Windows.UI.Xaml.Media.Imaging;
using Autofac;
using VLC_WinRT.Model;
using VLC_WinRT.Model.Video;
using VLC_WinRT.ViewModels;
using WinRTXamlToolkit.IO.Extensions;
using VLC_WinRT.Database;
using VLC_WinRT.Services.Interface;
using VLC_WinRT.Utils;
using VLC_WinRT.Model.Music;
using Windows.UI.Xaml;
using System.Linq.Expressions;

namespace VLC_WinRT.Helpers.VideoLibrary
{
    public class VideoLibrary
    {
        #region Video Library Indexation Logic

        private static async Task GetFilesFromSubFolders(StorageFolder folder, List<StorageFile> files)
        {
            try
            {
                var items = await folder.GetItemsAsync();
                foreach (IStorageItem storageItem in items)
                {
                    if (storageItem.IsOfType(StorageItemTypes.Folder))
                        await GetFilesFromSubFolders(storageItem as StorageFolder, files);
                    else if (storageItem.IsOfType(StorageItemTypes.File))
                    {
                        var file = storageItem as StorageFile;
                        if (VLCFileExtensions.VideoExtensions.Contains(file.FileType.ToLower()))
                        {
                            files.Add(file);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                LogHelper.Log(StringsHelper.ExceptionToString(e));
            }
        }

        private async Task<List<StorageFile>> GetMediaFromFolder(StorageFolder folder)
        {
#if WINDOWS_PHONE_APP
            var videoFiles = new List<StorageFile>();
            try
            {
                await GetFilesFromSubFolders(folder, videoFiles);
                return videoFiles;
            }
            catch (Exception ex)
            {
                LogHelper.Log("exception listing files");
                LogHelper.Log(ex.ToString());
            }
            return null;
#else
            var queryOptions = new QueryOptions { FolderDepth = FolderDepth.Deep };
            foreach (var type in VLCFileExtensions.VideoExtensions)
                queryOptions.FileTypeFilter.Add(type);
            var fileQueryResult = KnownFolders.VideosLibrary.CreateFileQueryWithOptions(queryOptions);
            var files = await fileQueryResult.GetFilesAsync();
            return files.ToList();
#endif
        }


        #endregion

    }
}