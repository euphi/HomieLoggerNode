/*
 * LoggerNode.cpp
 *
 *  Created on: 10.08.2016
 *      Author: ian
 */

#include "LoggerNode.h"
#include <Homie.hpp>

HomieSetting<const char*> LoggerNode::default_loglevel("loglevel", "default loglevel");  // id, description


LoggerNode::LoggerNode() :
		HomieNode("Log", "Logger", "Logger"), m_loglevel(DEBUG), logSerial(true), logJSON(true) {
	default_loglevel.setDefaultValue(levelstring[DEBUG].c_str()).setValidator([] (const char* candidate) {
		return convertToLevel(String(candidate)) != INVALID;
	});	
	advertise("log").setName("actual log output").setDatatype("String");
	advertise("Level").settable().setName("Loglevel").setDatatype("enum").setFormat(LoggerNode::getLevelStrings().c_str());
	advertise("LogSerial").settable().setName("enable/disable log to serial interface").setDatatype("boolean");
}

const String LoggerNode::levelstring[CRITICAL+1] = { "DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL" };

const String& LoggerNode::getLevelStrings() {
	static String* rc = NULL;
	if (!rc) rc = new String();
	for (int_fast8_t iLevel = DEBUG; iLevel <= CRITICAL; iLevel++) {
		rc->concat(levelstring[iLevel]);
		if (iLevel < CRITICAL)	rc->concat(',');
	}
	return *rc;

}

void LoggerNode::setup() {
	E_Loglevel loglevel = convertToLevel(String(default_loglevel.get()));
	if (loglevel == INVALID)
	{
		logf("LoggerNode", ERROR, "Invalid Loglevel in config (%s)", default_loglevel.get());
	} else {
		m_loglevel = loglevel;
		logf("LoggerNode", INFO, "Set loglvel to %s [%x]", levelstring[m_loglevel].c_str(), m_loglevel);
	}

}

void LoggerNode::onReadyToOperate() {
	setProperty("Level").send(levelstring[m_loglevel]);
	setProperty("LogSerial").send(logSerial? "true":"false");
}


void LoggerNode::log(const String& function, const E_Loglevel level, const String& text) const {
	if (!loglevel(level)) return;
	String message;
	String mqtt_path("log");
	if (Homie.isConnected()) {
		if (logJSON) {
			message.concat("{\"Level\": \"");
			message.concat(levelstring[level]);
			message.concat("\",\"Function\": \"");
			message.concat(function);
			message.concat("\",\"Message\": \"");
			message.concat(text);
			message.concat("\"}");
		} else {
			mqtt_path.concat('/');
			mqtt_path.concat(levelstring[level]);
			mqtt_path.concat('/');
			mqtt_path.concat(function);
			message = text;
		}
		setProperty(mqtt_path).send(message);
	}
	if (logSerial || !Homie.isConnected()) Serial.printf("%ld [%s]: %s:%s\n",millis(), levelstring[level].c_str(), function.c_str(), text.c_str());
}

void LoggerNode::logf(const String& function, const E_Loglevel level, const char* format, ...) const {
	if (!loglevel(level)) return;
	va_list arg;
	va_start(arg, format);
	char temp[100];
	size_t len = vsnprintf(temp, sizeof(temp), format, arg);
	va_end(arg);
	log(function, level, temp);
}


bool LoggerNode::handleInput(const HomieRange& range, const String& property,
		const String& value) {
	LN.logf("LoggerNode::handleInput()", LoggerNode::DEBUG,
			"property %s set to %s", property.c_str(), value.c_str());
	if (property.equals("Level") /* || property.equals("DefaultLevel") */) {
		E_Loglevel newLevel = convertToLevel(value);
		if (newLevel == INVALID) {
			logf(__PRETTY_FUNCTION__, WARNING , "Received invalid level %s.", value.c_str());
			return false;
		}
		if (property.equals("Level")) {
			m_loglevel = newLevel;
			logf(__PRETTY_FUNCTION__, INFO, "New loglevel set to %d", m_loglevel);
			setProperty("Level").send(levelstring[m_loglevel]);
			return true;
//		} else {  // DefaultLevel
//			//default_loglevel.set(levelstring[newLevel]);
//			//logf(__PRETTY_FUNCTION__, INFO, "New default loglevel set to %d", newLevel);
//			//setProperty("DefaultLevel").send(levelstring[newLevel]);
//			logf(__PRETTY_FUNCTION__, WARNING, "Setting persistent values via MQTT not yet supported by Homie");
		}
	} else if (property.equals("LogSerial")) {
		bool on = value.equalsIgnoreCase("ON") || value.equalsIgnoreCase("true");
		logSerial = on;
		LN.logf("LoggerNode::handleInput()", LoggerNode::INFO,
				"Receive command to switch 'Log to serial' %s.", on ? "On" : "Off");
		setProperty("LogSerial").send(on ? "true" : "false");
		return true;
	}
	logf(__PRETTY_FUNCTION__, ERROR,
			"Received invalid property %s with value %s", property.c_str(),	value.c_str());
	return false;
}

LoggerNode::E_Loglevel LoggerNode::convertToLevel(const String& level) {
	for (int_fast8_t iLevel = DEBUG; iLevel <= CRITICAL; iLevel++) {
		if (level.equalsIgnoreCase(levelstring[iLevel])) return static_cast<E_Loglevel>(iLevel);
	}
	return INVALID;

}
