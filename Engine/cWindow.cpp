#include <math.h>
#include "hWindow.hpp"
#include "hGlobal.hpp"
#include "hAssets.hpp"
#include "hInput.hpp"
#include <pugixml.hpp>

sf::RenderWindow Window::window;
Programmable Window::vars;
sf::Clock Window::deltaTimer;
std::vector<sf::Event> Window::events;
sf::RenderTexture Window::screen;
float Window::deltaTime;
std::vector<Window::CreditsGroup> Window::credits;

Window::CreditsGroup::CreditsGroup()
{
	title = "";
	creators.clear();
}

Window::CreditsGroup::CreditsGroup(sf::String name)
{
	title = name;
	creators.clear();
}

void Window::init(int argc, char *argv[])
{
	{
		auto assets = tr::splitStr(argv[1], "=");
		if (assets[0] == "assets")
		{
			AssetManager::path = assets[1];
			if (AssetManager::path[AssetManager::path.getSize() - 1] != '/') AssetManager::path += "/";
		}
		else AssetManager::path = "res/";
	}
	pugi::xml_document config;
	config.load_file(sf::String(AssetManager::path + "global/settings.trconf").toWideString().c_str());
	credits.clear();
	for (auto set : config.child(L"settings").children())
	{
		vars.setVar(set.name(), set.attribute(L"num").as_float());
		vars.setVar(set.name(), set.attribute(L"str").as_string());
	}
	for (auto group : config.child(L"credits").children())
	{
		CreditsGroup cg(group.attribute(L"name").as_string());
		for (auto creator : group.children())
		{
			cg.creators.push_back(creator.text().as_string());
		}
		credits.push_back(cg);
	}
	if (vars.getVar("Fullscreen"))
	{
		window.create(
			sf::VideoMode::getDesktopMode(),
			vars.getVar("Title").str,
			sf::Style::Fullscreen,
			sf::ContextSettings(24, 0, 0, 3, 3)
		);
	}
	else
	{
		window.create(
			sf::VideoMode(fmax(vars.getVar("SizeX"), 100), fmax(vars.getVar("SizeY"), 100)),
			vars.getVar("Title").str,
			sf::Style::Default,
			sf::ContextSettings(24, 0, 0, 3, 3)
		);
		if (hasVar("PosX") && hasVar("PosY")) window.setPosition({getVar("PosX"), getVar("PosY")});
	}
	if (getSize().x && getSize().y) screen.create(getSize().x, getSize().y);
	window.setVerticalSyncEnabled(vars.getVar("VSync"));
	window.setActive();
	if (vars.hasVar("Icon"))
	{
		sf::String str = vars.getVar("Icon").str;
		sf::Image img; img.loadFromFile(str);
		window.setIcon(img.getSize().x, img.getSize().y, img.getPixelsPtr());
	}
	// window.setMouseCursorVisible(false);
	for (int i = 1; i < argc; i++)
	{
		auto args = tr::splitStr(argv[i], "=");
		if (args[1] == "str") { vars.setVar(args[0], args[2]); }
		else if (args[1] == "num") { vars.setVar(args[0], std::stof(args[2].toAnsiString())); }
	}
}

void Window::update()
{
	deltaTime = deltaTimer.restart().asSeconds();
	setVar("FPS", 1.0f / deltaTime);
	setVar("mouseX", sf::Mouse::getPosition(window).x);
	setVar("mouseY", sf::Mouse::getPosition(window).y);
	setVar("cursorX", Input::getMousePos(true).x);
	setVar("cursorY", Input::getMousePos(true).y);
	window.setVerticalSyncEnabled(getVar("VSync"));
	events.clear();
	sf::Event e;
	while (window.pollEvent(e))
	{
		if (e.type == sf::Event::Closed) { window.close(); }
		else if (e.type == sf::Event::Resized)
		{
			window.setView(sf::View({0, 0, e.size.width, e.size.height}));
			screen.create(e.size.width, e.size.height);
		}
		else if (e.type == sf::Event::KeyPressed)
		{
			if (e.key.code == sf::Keyboard::F11)
			{
				setVar("Fullscreen", !getVar("Fullscreen"));
				if (vars.getVar("Fullscreen"))
				{
					window.create(
						sf::VideoMode::getDesktopMode(),
						vars.getVar("Title").str,
						sf::Style::Fullscreen,
						sf::ContextSettings(24, 0, 0, 3, 3)
					);
				}
				else
				{
					window.create(
						sf::VideoMode(vars.getVar("SizeX"), vars.getVar("SizeY")),
						vars.getVar("Title").str,
						sf::Style::Default,
						sf::ContextSettings(24, 0, 0, 3, 3)
					);
					if (hasVar("PosX") && hasVar("PosY")) window.setPosition({getVar("PosX"), getVar("PosY")});
				}
				screen.create(getSize().x, getSize().y);
			}
		}
		events.push_back(e);
	}
}

sf::Event Window::getEvent(sf::Event::EventType type)
{
	for (int i = 0; i < events.size(); i++) { if (events[i].type == type) { return events[i]; } }
	return sf::Event();
}

sf::Event Window::waitEvent(sf::Event::EventType type)
{
	sf::Event e;
	while (e.type != type)
	{
		Window::update();
		e = getEvent(type);
	}
	return e;
}

void Window::clear()
{
	auto clr = tr::splitStr(getVar("clearClr").str, "-");
	sf::Color clearClr = sf::Color::Black;
	if (clr.size() > 0) clearClr = sf::Color(
		std::stoi(clr[0].toAnsiString()), std::stoi(clr[1].toAnsiString()),
		std::stoi(clr[2].toAnsiString()), std::stoi(clr[3].toAnsiString())
	);
	window.clear(clearClr);
	screen.clear(clearClr);
}

sf::Texture Window::capture()
{
	sf::Texture t;
	t.update(window);
	return t;
}

void Window::draw(sf::Drawable &obj, bool drawUI, const sf::RenderStates states)
{
	if (getVar("showCredits")) return;
	if (drawUI) window.draw(obj, states);
	else screen.draw(obj, states);
}

void Window::drawScreen()
{
	screen.display();
	auto s = sf::Sprite(screen.getTexture());
	draw(s);
}

void Window::setView(sf::View view)
{
	screen.setView(view);
}

void Window::close() { window.close(); screen.clear(); }
void Window::display()
{
	if (getVar("showCredits"))
	{
		auto font = *AssetManager::getFont("global/font.ttf");
		int count = 0, current = 0;
		for (int i = 0; i < credits.size(); i++)
		{
			count++;
			count += credits[i].creators.size();
		}
		float y_diff = getSize().y / (count + 1);
		for (int i = 0; i < credits.size(); i++)
		{
			auto *g = &credits[i];
			sf::Text txt(g->title + ":", font, 24);
			txt.setPosition(getSize().x / 2, y_diff * ++current);
			txt.setFillColor(sf::Color::White);
			float c = tr::clamp(tr::lerp(getVar("credit"), 1, getDeltaTime() * 5), 0, 1);
			txt.setFillColor(sf::Color(0, c * 196, c * 22, c * 255));
			setVar("credit", c);
			txt.setOrigin(txt.getLocalBounds().getSize() / 2.0f);
			window.draw(txt);
			for (int j = 0; j < g->creators.size(); j++)
			{
				txt.setFillColor(sf::Color(c * 255, c * 255, c * 255, c * 255));
				txt.setCharacterSize(20);
				txt.setString(g->creators[j]);
				txt.setOrigin(txt.getLocalBounds().getSize() / 2.0f);
				txt.setPosition(getSize().x / 2, y_diff * ++current);
				window.draw(txt);
			}
		}
		if (Input::isKeyJustPressed(sf::Keyboard::Escape))
		{
			setVar("credit", 0);
			setVar("showCredits", 0);
		}
	}
	window.display();
}
bool Window::hasEvent(sf::Event::EventType type) { return getEvent(type).type == type; }
void Window::addEvent(sf::Event e) { events.push_back(e); }
float Window::getDeltaTime() { return deltaTime; }
bool Window::isOpen() { return window.isOpen(); }
void Window::resetTime() { deltaTime = 0; deltaTimer.restart(); }
sf::Vector2f Window::getSize() { return (sf::Vector2f)window.getSize(); }
bool Window::hasFocus() { return window.hasFocus(); }
void Window::clearVars() { vars.clear(); }
void Window::setVar(sf::String name, sf::String value) { vars.setVar(name, value); }
void Window::setVar(sf::String name, float value) { vars.setVar(name, value); }
Programmable::Variable Window::getVar(sf::String name) { return vars.getVar(name); }
bool Window::hasVar(sf::String name) { return vars.hasVar(name); }
sf::RenderTarget *Window::getRenderTarget() { return &window; }
sf::RenderTexture *Window::getScreen() { return &screen; }
Programmable *Window::getProgrammable() { return &vars; }

void Window::setTitle(sf::String title)
{
	window.setTitle(title);
	setVar("Title", title);
}

sf::String Window::getTitle()
{
	return getVar("Title");
}