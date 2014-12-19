/*
** Taiga
** Copyright (C) 2010-2014, Eren Okka
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <map>
#include <assert.h>

#include "track/stream_provider_parser.h"
#include "base/string.h"
#include "base/accessibility.h"
#include "base/xml.h"
#include "taiga/path.h"
#include "base/html_fetch.h"

using namespace Track;

std::wstring Track::taigaParseSourceTypeToWString(ParseSourceType type)
{
	static std::map<ParseSourceType, std::wstring> s_parseTypeStrings;
	if (s_parseTypeStrings.empty()) {
		s_parseTypeStrings[ParseSourceType::Invalid]     = L"invalid";
		s_parseTypeStrings[ParseSourceType::WindowTitle] = L"window_title";
		s_parseTypeStrings[ParseSourceType::Url]         = L"url";
		s_parseTypeStrings[ParseSourceType::HtmlSource]  = L"html_source";
	}

	assert(s_parseTypeStrings.find(type) != s_parseTypeStrings.end()
		&& "Please add new types to map initalization.");

	return s_parseTypeStrings[type];
}
ParseSourceType Track::taigaWStringToParseSourceType(const std::wstring& type)
{
	static std::map<std::wstring, ParseSourceType> s_parseTypeStrings;
	if (s_parseTypeStrings.empty()) {
		s_parseTypeStrings[L"invalid"]      = ParseSourceType::Invalid;
		s_parseTypeStrings[L"window_title"] = ParseSourceType::WindowTitle;
		s_parseTypeStrings[L"url"]          = ParseSourceType::Url;
		s_parseTypeStrings[L"html_source"]  = ParseSourceType::HtmlSource;
	}

	assert(s_parseTypeStrings.find(type) != s_parseTypeStrings.end()
		&& "Please add new types to map initalization.");

	auto it = s_parseTypeStrings.find(type);
	if (it == s_parseTypeStrings.end())
		return ParseSourceType::Invalid;
	else
		return it->second;
}

const std::wstring StreamProviderParserPrototype::parseTitle() const
{
	std::wstring title  = m_episodeTitleParsing();
	std::wstring number = m_episodeNumberParsing();
	if (!number.empty())
		title += L" Episode " + number;
	return title;
}

StreamProviderParserPrototype* StreamProviderParserPrototype::createNewInstance(const std::wstring& url, const std::wstring& title) const
{
	StreamProviderParserPrototype* newInstance = new StreamProviderParserPrototype(*this);
	newInstance->m_parsingSource.url = url;
	newInstance->m_parsingSource.title = title;
	newInstance->m_episodeTitleParsing = m_episodeTitleParsing;
	newInstance->m_episodeTitleParsing.parsingSource = newInstance->m_parsingSource;
	newInstance->m_episodeNumberParsing = m_episodeNumberParsing;
	newInstance->m_episodeNumberParsing.parsingSource = newInstance->m_parsingSource;
	return newInstance;
}

bool StreamProviderParserPrototype::supportsUrl(const std::wstring& url) const
{
	return SearchRegex(url, m_regexUrlSupported);
}

const std::wstring  StreamProviderParserPrototype::getHumanReadableName() const
{
	return m_humanReadableName;
}

void StreamProviderParserPrototype::setRegexUrlSupported(const std::wstring& regexPattern)
{
	m_regexUrlSupported = regexPattern;
}

void StreamProviderParserPrototype::setEpisodeTitleParsing(const std::wstring& regexPattern, ParseSourceType parseType)
{
	m_episodeTitleParsing.regexPattern = regexPattern;
	m_episodeTitleParsing.parseType = parseType;
}

void StreamProviderParserPrototype::setEpisodeNumberParsing(const std::wstring& regexPattern, ParseSourceType parseType)
{
	m_episodeNumberParsing.regexPattern = regexPattern;
	m_episodeNumberParsing.parseType = parseType;
}

bool StreamProviderParserPrototype::ParsingElement::isValid() const
{
	return (parseType != ParseSourceType::Invalid && regexPattern != INVALID);
}

std::wstring StreamProviderParserPrototype::ParsingElement::operator() () const
{
	if (!isValid())
		return std::wstring();

	switch (parseType)
	{
	case ParseSourceType::WindowTitle: return FirstMatchRegex(parsingSource.title, regexPattern);
	case ParseSourceType::Url:         return FirstMatchRegex(parsingSource.url, regexPattern);
	case ParseSourceType::HtmlSource:  return parseHtmlSource(parsingSource.url, regexPattern);
	default:
		return std::wstring();
	}
}

std::wstring StreamProviderParserPrototype::ParsingElement::parseHtmlSource(const std::wstring& url, const std::wstring& pattern) const
{
	std::wstring htmlSource = Base::taigaFetchHtmlPageSource(url);
	return FirstMatchRegex(htmlSource, pattern);
}

StreamProviderParserPrototype::ParsingElement&
StreamProviderParserPrototype::ParsingElement::operator= (const ParsingElement& other)
{
	//protect against self assigement
	if (this == &other)
		return *this;

	//do the assigement
	parseType = other.parseType;
	regexPattern = other.regexPattern;
	parsingSource = other.parsingSource;

	//return itself
	return *this;
}

void StreamProviderParserPrototype::setHumanReadableName(const std::wstring& humanReadableName)
{
	m_humanReadableName = humanReadableName;
}

BOOL StreamProviderParserPrototype::isEnabled() const
{
	return m_enabled;
}

void StreamProviderParserPrototype::setEnabled(BOOL enabled)
{
	m_enabled = enabled;
}


StreamProviderParserFactory::StreamProviderParserFactory()
{
}

StreamProviderParserFactory::~StreamProviderParserFactory()
{
	//delete prototypes because StreamProviderParserManager is owner
	for each (StreamProviderParserPrototype* prototype in m_streamProvidersParsersPrototypes)
		delete prototype;
}

bool StreamProviderParserFactory::loadPrototypes()
{
	xml_document document;
	std::wstring path = taiga::GetPath(taiga::kPathMedia);
	xml_parse_result parse_result = document.load_file(path.c_str());

	if (parse_result.status != pugi::status_ok)
		return false;

	// Read provider list
	xml_node mediaProviders = document.child(L"media_providers");
	foreach_xmlnode_(provider, mediaProviders, L"provider") {
		auto xmlProvider = new StreamProviderParserPrototype;
		xmlProvider->setHumanReadableName(XmlReadStrValue(provider, L"name"));
		xmlProvider->setRegexUrlSupported(XmlReadStrValue(provider, L"url"));
		//enabled
		xmlProvider->setEnabled(XmlReadIntValue(provider, L"enabled") != 0);
		addStreamProviderParserPrototype(xmlProvider);
		//get title
		xml_node title = provider.child(L"title");
		if (title) {
			std::wstring parseTypeString = title.attribute(L"source").as_string();
			ParseSourceType parseType = Track::taigaWStringToParseSourceType(parseTypeString);
			std::wstring titleRegExPattern = XmlReadStrValue(provider, L"title");
			xmlProvider->setEpisodeTitleParsing(titleRegExPattern, parseType);
		}
		//get episode number
		xml_node episode = provider.child(L"episode_number");
		if (episode) {
			std::wstring parseTypeString = episode.attribute(L"source").as_string();
			ParseSourceType parseType = Track::taigaWStringToParseSourceType(parseTypeString);
			std::wstring episodeRegExPattern = XmlReadStrValue(provider, L"episode_number");
			xmlProvider->setEpisodeNumberParsing(episodeRegExPattern, parseType);
		}
	}
	return true;
}

void StreamProviderParserFactory::addStreamProviderParserPrototype(StreamProviderParserPrototype* streamProviderParserPrototype)
{
	m_streamProvidersParsersPrototypes.push_back(streamProviderParserPrototype);
}

const std::vector<StreamProviderParserPrototype*>& StreamProviderParserFactory::getAllStreamProviderParserPrototypes() const
{
	return m_streamProvidersParsersPrototypes;
}

StreamProviderParserRaii StreamProviderParserFactory::createStreamProviderParser(const std::wstring& url, const std::wstring& title) const
{
	IStreamProviderParser* newParser = nullptr;
	for each (StreamProviderParserPrototype* prototype in m_streamProvidersParsersPrototypes) {
		if (prototype->supportsUrl(url)) {
			newParser = prototype->createNewInstance(url, title);
			break;
		}
	}
	return StreamProviderParserRaii(newParser);
}