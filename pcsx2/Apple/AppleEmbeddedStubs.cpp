// SPDX-FileCopyrightText: 2002-2026 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

// Stubs for what iOS and tvOS do not have. MacOSStubs.cpp next to this one is
// the macOS equivalent and excludes itself on the embedded platforms; this is
// the other half, and it is a much shorter list because most of what macOS has
// to stub out for a UI-less build is present here anyway.
//
// Everything below is something the platform genuinely does not have, rather
// than something this build leaves out: there is no optical drive to read a
// disc from, and no NSSound to play an achievement chime through.

#include "PrecompiledHeader.h"

#include "CDVD/CDVDdiscReader.h"

#include "common/HostSys.h"

// Sound playback. On macOS this is CocoaTools.mm, which is AppKit and so not
// built here (see common/CMakeLists.txt); the caller is the achievement chime,
// which treats a false as "no sound played".
bool Common::PlaySoundAsync(const char* path)
{
	return false;
}

// Optical drive. The Darwin disc sources are IOKit, which these platforms do
// not have, so pcsx2/CMakeLists.txt leaves them out - and CDVDdiscReader, which
// is built everywhere, names all of this. An empty drive list is the honest
// answer: a phone or a TV box has no disc to put in.
std::vector<std::string> GetOpticalDriveList()
{
	return {};
}

void GetValidDrive(std::string& drive)
{
	drive.clear();
}

IOCtlSrc::IOCtlSrc(std::string filename)
{
}

IOCtlSrc::~IOCtlSrc()
{
}

bool IOCtlSrc::Reopen(Error* error)
{
	return false;
}

bool IOCtlSrc::DiscReady()
{
	return false;
}

u32 IOCtlSrc::GetSectorCount() const
{
	return 0;
}

s32 IOCtlSrc::GetMediaType() const
{
	return 0;
}

const std::vector<toc_entry>& IOCtlSrc::ReadTOC() const
{
	static const std::vector<toc_entry> empty;
	return empty;
}

bool IOCtlSrc::ReadSectors2048(u32 sector, u32 count, u8* buffer) const
{
	return false;
}

bool IOCtlSrc::ReadSectors2352(u32 sector, u32 count, u8* buffer) const
{
	return false;
}

bool IOCtlSrc::ReadTrackSubQ(cdvdSubQ* subq) const
{
	return false;
}

u32 IOCtlSrc::GetLayerBreakAddress() const
{
	return 0;
}
