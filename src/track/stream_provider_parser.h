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


/*
Provides parsers for stream providers to retrieve series/movie title
and or episode number from webbrowser by given url and window/tab title.

The parsers can be defined in the media.xml:

Example:
<media_providers>
	<!-- Crunchyroll -->
	<provider>
		<name>Crunchyroll</name>
		<enabled>1</enabled>
		<url>crunchyroll.+(episode-[0-9]+)?.*(movie)?-[0-9]+</url>
		<title source="window_title">Crunchyroll - Watch (.+)( - Movie - Movie)?</title>
	</provider>
</media_providers>

<name>    := Human readable name of the provider. Used for gui.
<enabled> := Weather the parser of the given provider is active or not.
<url>     := regex pattern if a given url is supported by the parser.
<title soure=""> := regex pattern with capture group to get the episode title.
<episode_number source=""> := regex patern with capture group to get episode number.

source := defines the source to apply the regex pattern.
	Valid sources are:
	 * window_title - The title of the Window/Tab.
	 * url - Url of the page.
	 * html_source - source of the html at the url.
*/

#ifndef MEDIASTREAMINGPROVIDER_H
#define MEDIASTREAMINGPROVIDER_H

#include <string>
#include <vector>

#define INVALID L"INVALID"

typedef int BOOL;

namespace Track
{
	enum class ParseSourceType
	{
		Invalid            = 0,
		WindowTitle        = (1 << 0), //use window title as source 
		Url                = (1 << 1), //use Url as source
		HtmlSource         = (1 << 2) //use html source code as source
	};

	std::wstring taigaParseSourceTypeToWString(ParseSourceType type);
	
	ParseSourceType taigaWStringToParseSourceType(const std::wstring& type);

	class IStreamProviderParser
	{
	public:		
		virtual const std::wstring parseTitle() const = 0;

	protected:
		IStreamProviderParser() {}

		class ParsingSource
		{
		public:
			ParsingSource() : url(INVALID), title(INVALID) {}
			std::wstring url;
			std::wstring title;
		} m_parsingSource;
	};

	class StreamProviderParserPrototype : public IStreamProviderParser
	{
	public:
		//ctor
		StreamProviderParserPrototype()
			: m_enabled(0)
			, m_regexUrlSupported(INVALID)
			, m_episodeNumberParsing(m_parsingSource)
			, m_episodeTitleParsing(m_parsingSource) {};

		const std::wstring parseTitle() const;
		const std::wstring getHumanReadableName() const;
		
		StreamProviderParserPrototype* createNewInstance(const std::wstring& url, const std::wstring& title) const;
		bool supportsUrl(const std::wstring& url) const;
		
		void setRegexUrlSupported(const std::wstring& regexPattern);
		void setEpisodeTitleParsing(const std::wstring& regexPattern, ParseSourceType parseType);
		void setEpisodeNumberParsing(const std::wstring& regexPattern, ParseSourceType parseType);
		void setHumanReadableName(const std::wstring& humanReadableName);

		BOOL isEnabled() const;
		void setEnabled(BOOL enabled);

	private:

		BOOL m_enabled;
		std::wstring m_regexUrlSupported;
		std::wstring m_humanReadableName;

		class ParsingElement
		{
		public:
			//ctor
			ParsingElement(ParsingSource& parsingSource)
				: regexPattern(INVALID)
				, parseType(ParseSourceType::Invalid)
				, parsingSource(parsingSource) {}
			//copy operator
			ParsingElement& operator= (const ParsingElement& other);
			//validity
			bool isValid() const;
			//parsing
			std::wstring operator() () const;
			//members
			std::wstring    regexPattern;
			ParseSourceType parseType;
			ParsingSource&  parsingSource;
		private: 
			std::wstring parseHtmlSource(const std::wstring& url, const std::wstring& pattern) const;
		};

		ParsingElement m_episodeTitleParsing;
		ParsingElement m_episodeNumberParsing;
	};


	class StreamProviderParserRaii
	{
	public:
		//ctor
		StreamProviderParserRaii(IStreamProviderParser* parser)
			: m_parser(parser)
			, m_handOverDeletion(true)
			, m_delete(true) {}
		//copy ctor
		StreamProviderParserRaii(StreamProviderParserRaii& other)
		{
			m_parser = other.m_parser;
			m_delete = other.m_handOverDeletion ? true : false;
			other.m_delete = !other.m_handOverDeletion;
		}
		//dtor
		~StreamProviderParserRaii()
		{
			if (m_delete)
				delete m_parser;
		}
		//access operator
		const IStreamProviderParser* operator ->() const
		{
			return m_parser;
		}
		//bool cast operator for validity check
		operator bool() const
		{
			return (m_parser != nullptr);
		}
		//set handover of delete
		void setHandOverDeletion(bool flag)
		{
			m_handOverDeletion = flag;
		}

	private:
		IStreamProviderParser* m_parser;
		bool m_handOverDeletion;
		bool m_delete;
	};


	class StreamProviderParserFactory
	{
	public:
		StreamProviderParserFactory();
		~StreamProviderParserFactory();

		bool loadPrototypes();
		const std::vector<StreamProviderParserPrototype*>& getAllStreamProviderParserPrototypes() const;
		
		//StreamProviderParserFactory takes ownership of prototypes
		void addStreamProviderParserPrototype(StreamProviderParserPrototype* streamProviderParserPrototype);
		
		StreamProviderParserRaii createStreamProviderParser(const std::wstring& url, const std::wstring& title) const;

	private:

		std::vector<StreamProviderParserPrototype*> m_streamProvidersParsersPrototypes;

	};

}// Track

#endif