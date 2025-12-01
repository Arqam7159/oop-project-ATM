#ifndef BUTTON_H
#define BUTTON_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class Button {
private:
    sf::RectangleShape shape;
    sf::Text text;
    std::function<void()> action;
    sf::Color normalColor;
    sf::Color hoverColor;
    bool isHovered;

public:
    Button(const std::string& label, const sf::Font& font, 
           sf::Vector2f size, sf::Vector2f position) {
        shape.setSize(size);
        shape.setPosition(position);
        normalColor = sf::Color(50, 100, 200);
        hoverColor = sf::Color(70, 120, 220);
        shape.setFillColor(normalColor);
        shape.setOutlineThickness(2);
        shape.setOutlineColor(sf::Color::White);
        
        text.setFont(font);
        text.setString(label);
        text.setCharacterSize(18);
        text.setFillColor(sf::Color::White);
        
        // Center text in button
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition(
            position.x + (size.x - textBounds.width) * 0.5f - textBounds.left,
            position.y + (size.y - textBounds.height) * 0.5f - textBounds.top
        );
        
        isHovered = false;
    }

    void setAction(std::function<void()> func) {
        action = func;
    }

    bool contains(sf::Vector2f point) const {
        return shape.getGlobalBounds().contains(point);
    }

    void update(sf::Vector2f mousePos) {
        if (contains(mousePos)) {
            shape.setFillColor(hoverColor);
            isHovered = true;
        } else {
            shape.setFillColor(normalColor);
            isHovered = false;
        }
    }

    void trigger() {
        if (action) action();
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape);
        window.draw(text);
    }
};


#endif // BUTTON_H
