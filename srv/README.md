# 🌱 Sensora – Capteur Environnemental

---

## ⚙️ Prérequis

- Système : Linux (Ubuntu recommandé)
- Logiciels : [Docker](https://docs.docker.com/get-docker/) et [Docker Compose](https://docs.docker.com/compose/install/) installés sur le serveur

---

## 📁 Arborescence des fichiers

> ⚠️ **Important** : Tous les fichiers doivent être placés **exactement** dans le dossier `/srv/` sur le serveur pour que les volumes soient correctement montés.

```
/srv/
├── sites/
│   ├── site1/               ← Site principal
│   │   └── html/
│   │       ├── index.html
│   │       ├── styles.css
│   │       ├── nav.js
│   │       ├── filter.js
│   │       ├── chart.js
│   │       ├── realtime.js
│   │       └── images/
│   │           └── logo.png
│   └── site2/               ← (Site de test, facultatif)
│       └── html/
├── config/                  ← Fichiers de configuration
│   ├── mosquitto/
│   │   └── mosquitto.conf
│   └── telegraf/
│       └── telegraf.conf
└── websockets/
    └── script.py            ← Script Python utilisé par le service WebSockets
```

> 🛠️ **Note** : Crée manuellement les dossiers si besoin, et donne les bonnes permissions (`root`) pour éviter tout souci d'accès depuis les conteneurs.

---

## 🐳 Construction de l’image WebSockets

1. Place le fichier `dockerfile-websockets.txt` n’importe où sur le serveur (ex : dans le dossier du projet).
2. Construis l’image avec :

```bash
docker build -t websockets -f dockerfile-websockets.txt .
```

---

## 🚀 Démarrage initial des services

Lance les services avec Docker Compose :

```bash
docker-compose up -d
```

Cela va créer tous les conteneurs, **même si certains (comme `telegraf` ou `websockets`) échouent temporairement** à cause de l'absence de token InfluxDB valide.

---

## 🔐 Générer et insérer les tokens InfluxDB

1. Une fois InfluxDB lancé, accédez à l’interface web :

```
http://<ADRESSE_IP_DU_SERVEUR>:8086
```

2. Créez un compte, une organisation, un bucket et un **API Token**. Pour l'organisation mettre : "myorg" et le bucket : "sensors".

3. Modifiez ensuite le fichier `docker-compose.yml` :

```yaml
  environment:
    - INFLUX_TOKEN=VOTRE_TOKEN_Ici
```

4. Redémarrez les services `telegraf` et `websockets` :

```bash
docker-compose up -d --force-recreate telegraf websockets
```

---

## 🛑 Arrêter les services

Pour tout arrêter proprement (sans supprimer les données) :

```bash
docker-compose down
```

---

## 🌐 Accès aux services

> Remplacez `<ADRESSE_IP_DU_SERVEUR>` par l’adresse IP de votre machine.

| Service     | Adresse par défaut                            |
|-------------|-----------------------------------------------|
| Site web    | http://<ADRESSE_IP_DU_SERVEUR>/site1/         |
| InfluxDB    | http://<ADRESSE_IP_DU_SERVEUR>:8086           |
| Portainer   | https://<ADRESSE_IP_DU_SERVEUR>:9443          |

---

✅ Ton infrastructure est maintenant prête à l’emploi !
