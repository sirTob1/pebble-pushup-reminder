module.exports = [
  {
    "type": "heading",
    "defaultValue": "Pushups Configuration"
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "General Settings"
      },
      {
        "type": "select",
        "messageKey": "LANGUAGE",
        "defaultValue": "0",
        "label": "Language / Sprache",
        "options": [
          { 
            "label": "Deutsch", 
            "value": "0" 
          },
          { 
            "label": "English", 
            "value": "1" 
          }
        ]
      },
      {
        "type": "slider",
        "messageKey": "DAILY_GOAL",
        "defaultValue": 30,
        "label": "Baseline Daily Goal",
        "description": "The base number of pushups you want to do per day.",
        "min": 10,
        "max": 500,
        "step": 5
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Reminders & Active Hours"
      },
      {
        "type": "slider",
        "messageKey": "REMINDER_INTERVAL",
        "defaultValue": 60,
        "label": "Reminder Interval (Minutes)",
        "description": "How often you want to be reminded.",
        "min": 15,
        "max": 240,
        "step": 15
      },
      {
        "type": "slider",
        "messageKey": "ACTIVE_START_HOUR",
        "defaultValue": 8,
        "label": "Active Start Hour",
        "description": "When should the reminders start? (0-23)",
        "min": 0,
        "max": 23,
        "step": 1
      },
      {
        "type": "slider",
        "messageKey": "ACTIVE_END_HOUR",
        "defaultValue": 20,
        "label": "Active End Hour",
        "description": "When should the reminders stop? (0-23)",
        "min": 0,
        "max": 23,
        "step": 1
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
