import firebase_admin
from firebase_admin import credentials, db

# Pad naar je service account JSON bestand
cred = credentials.Certificate('pool-671d1-firebase-adminsdk-fbsvc-f5e2ff230f.json')

# Initialiseer de app met de service account en database URL
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://pool-671d1-default-rtdb.europe-west1.firebasedatabase.app/'
})

# Lees de firmwareversie uit het bestand
with open('firmware_version.txt') as f:
    version = f.read().strip()

# Schrijf de nieuwe versie naar Firebase
ref = db.reference('firmware/latest_version')
ref.set(version)
print('Firmwareversie succesvol geüpload:', version)
