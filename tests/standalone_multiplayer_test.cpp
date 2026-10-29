#include "test_framework.h"
#include "../network_manager.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <iostream>

void processQtEvents(int ms = 50) {
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

void runMultiplayerTests() {
    TEST_SUITE("Multiplayer LAN & Secret Code Verification");

    // -------------------------------------------------------------
    // Test 1: Host setup with group secret code
    // -------------------------------------------------------------
    TEST_CASE("MP1.1: Host initialization with secret code") {
        NetworkManager host;
        bool ok = host.startHosting("KART7", "ALICE", 0);
        ASSERT_TRUE(ok);
        ASSERT_EQ(host.getRole(), NetworkRole::HOST);
        ASSERT_EQ(host.getSecretCode(), QString("KART7"));
        ASSERT_EQ(host.getLocalPlayerName(), QString("ALICE"));
        ASSERT_EQ(host.getLocalPlayerId(), 0);
        ASSERT_EQ(host.getPlayers().size(), 1);
        ASSERT_TRUE(host.isConnected());
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 2: Case insensitivity of secret code
    // -------------------------------------------------------------
    TEST_CASE("MP1.2: Secret code case insensitivity normalization") {
        NetworkManager host;
        host.startHosting("turbo9", "BOB", 1);
        ASSERT_EQ(host.getSecretCode(), QString("TURBO9"));
        ASSERT_EQ(host.getSelectedTrack(), 1);
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 3: Client Join and Handshake with Secret Code over LAN/Localhost
    // -------------------------------------------------------------
    TEST_CASE("MP1.3: Host and Client handshake via Secret Code") {
        NetworkManager host;
        bool hostOk = host.startHosting("MATCH5", "HOST_P1", 0);
        ASSERT_TRUE(hostOk);
        processQtEvents(50);

        NetworkManager client;
        bool clientOk = client.startJoining("MATCH5", "CLIENT_P2", "127.0.0.1", host.getServerPort());
        ASSERT_TRUE(clientOk);

        // Allow connection to establish
        for (int i = 0; i < 20; ++i) {
            processQtEvents(50);
            if (client.isConnected() && client.getPlayers().size() >= 2) break;
        }

        ASSERT_TRUE(client.isConnected());
        ASSERT_EQ(client.getPlayers().size(), 2);
        ASSERT_EQ(host.getPlayers().size(), 2);

        // Verify Player 1 (Host) and Player 2 (Client)
        if (host.getPlayers().size() >= 2 && client.getPlayers().size() >= 2) {
            ASSERT_EQ(host.getPlayers()[0].name, QString("HOST_P1"));
            ASSERT_EQ(host.getPlayers()[1].name, QString("CLIENT_P2"));
            ASSERT_EQ(client.getPlayers()[0].name, QString("HOST_P1"));
            ASSERT_EQ(client.getPlayers()[1].name, QString("CLIENT_P2"));
            ASSERT_EQ(client.getLocalPlayerId(), 1);
        }

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 4: Rejection of Invalid Secret Code
    // -------------------------------------------------------------
    TEST_CASE("MP1.4: Client join rejected with mismatched Secret Code") {
        NetworkManager host;
        host.startHosting("SECRET_A", "HOST", 0);
        processQtEvents(50);

        NetworkManager client;
        client.startJoining("WRONG_CODE", "INTRUDER", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 15; ++i) {
            processQtEvents(50);
            if (!client.isConnected()) break;
        }

        // Intruder should not be admitted to host lobby
        ASSERT_EQ(host.getPlayers().size(), 1);

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 5: In-Lobby Character & Ready Synchronization
    // -------------------------------------------------------------
    TEST_CASE("MP1.5: Character selection & ready status synchronization") {
        NetworkManager host;
        host.startHosting("CODE88", "PLAYER1", 0);
        processQtEvents(30);

        NetworkManager client;
        client.startJoining("CODE88", "PLAYER2", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 20; ++i) {
            processQtEvents(40);
            if (client.isConnected() && client.getPlayers().size() == 2) break;
        }

        // Client chooses Bowser (ID 1)
        client.setCharacter(1);
        processQtEvents(50);

        // Host chooses Peach (ID 2)
        host.setCharacter(2);
        processQtEvents(50);

        ASSERT_TRUE(host.getPlayer(0) != nullptr);
        ASSERT_TRUE(host.getPlayer(1) != nullptr);
        ASSERT_TRUE(client.getPlayer(0) != nullptr);
        ASSERT_TRUE(client.getPlayer(1) != nullptr);

        if (host.getPlayer(0) && host.getPlayer(1) && client.getPlayer(0) && client.getPlayer(1)) {
            ASSERT_EQ(host.getPlayer(0)->characterId, 2);
            ASSERT_EQ(host.getPlayer(1)->characterId, 1);
            ASSERT_EQ(client.getPlayer(0)->characterId, 2);
            ASSERT_EQ(client.getPlayer(1)->characterId, 1);

            // Check ready states
            ASSERT_TRUE(host.getPlayer(0)->isReady);
            ASSERT_FALSE(host.allPlayersReady());

            client.setReady(true);
            processQtEvents(50);

            ASSERT_TRUE(client.getPlayer(1)->isReady);
            ASSERT_TRUE(host.getPlayer(1)->isReady);
            ASSERT_TRUE(host.allPlayersReady());
        }

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 6: Race Start & Track Propagation
    // -------------------------------------------------------------
    TEST_CASE("MP1.6: Host race start triggers clients on selected track") {
        NetworkManager host;
        host.startHosting("RACE_GO", "HOST", 2); // Bowser Castle
        processQtEvents(30);

        NetworkManager client;
        client.startJoining("RACE_GO", "CLIENT", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 20; ++i) {
            processQtEvents(40);
            if (client.isConnected() && client.getPlayers().size() == 2) break;
        }

        int clientReceivedTrack = -1;
        QObject::connect(&client, &NetworkManager::raceStartTriggered, [&](int track) {
            clientReceivedTrack = track;
        });

        client.setReady(true);
        processQtEvents(40);

        host.startRace();
        processQtEvents(60);

        ASSERT_EQ(clientReceivedTrack, 2);

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 7: Live Kart Position Synchronization
    // -------------------------------------------------------------
    TEST_CASE("MP1.7: In-race real-time kart state telemetry broadcast") {
        NetworkManager host;
        host.startHosting("TELEMETRY", "HOST", 0);
        processQtEvents(30);

        NetworkManager client;
        client.startJoining("TELEMETRY", "CLIENT", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 20; ++i) {
            processQtEvents(40);
            if (client.isConnected() && client.getPlayers().size() == 2) break;
        }

        // Host drives to (550.0, 210.5)
        host.broadcastLocalKartState(550.0, 210.5, 1.57, 120.0, 2, false, 0.0, false, true, 3);
        processQtEvents(50);

        const NetPlayer *hostOnClient = client.getPlayer(0);
        ASSERT_TRUE(hostOnClient != nullptr);
        if (hostOnClient) {
            ASSERT_EQ(hostOnClient->x, 550.0);
            ASSERT_EQ(hostOnClient->z, 210.5);
            ASSERT_EQ(hostOnClient->currentLap, 2);
            ASSERT_TRUE(hostOnClient->isDrifting);
            ASSERT_EQ(hostOnClient->heldItem, 3);
        }

        // Client drives to (600.0, 300.0)
        client.broadcastLocalKartState(600.0, 300.0, 0.0, 140.0, 1, false, 0.0, true, false, 1);
        processQtEvents(50);

        const NetPlayer *clientOnHost = host.getPlayer(1);
        ASSERT_TRUE(clientOnHost != nullptr);
        if (clientOnHost) {
            ASSERT_EQ(clientOnHost->x, 600.0);
            ASSERT_EQ(clientOnHost->z, 300.0);
            ASSERT_TRUE(clientOnHost->isSpinning);
        }

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 8: End-of-Race Rankings & Placement Determination
    // -------------------------------------------------------------
    TEST_CASE("MP1.8: Race finish ranking calculation and leaderboard") {
        NetworkManager host;
        host.startHosting("FINISH7", "ALICE", 0);
        processQtEvents(30);

        NetworkManager client;
        client.startJoining("FINISH7", "BOB", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 20; ++i) {
            processQtEvents(40);
            if (client.isConnected() && client.getPlayers().size() == 2) break;
        }

        // Bob crosses finish line first at 74.52 seconds
        client.notifyLocalFinish(74.52);
        processQtEvents(50);

        // Alice crosses finish line second at 78.10 seconds
        host.notifyLocalFinish(78.10);
        processQtEvents(50);

        // Verify placements
        ASSERT_TRUE(host.getPlayer(0) != nullptr);
        ASSERT_TRUE(host.getPlayer(1) != nullptr);
        ASSERT_TRUE(client.getPlayer(0) != nullptr);
        ASSERT_TRUE(client.getPlayer(1) != nullptr);

        if (host.getPlayer(0) && host.getPlayer(1) && client.getPlayer(0) && client.getPlayer(1)) {
            ASSERT_TRUE(host.getPlayer(1)->raceFinished);
            ASSERT_TRUE(host.getPlayer(0)->raceFinished);
            ASSERT_EQ(host.getPlayer(1)->placement, 1); // Bob is 1st
            ASSERT_EQ(host.getPlayer(0)->placement, 2); // Alice is 2nd

            ASSERT_TRUE(client.getPlayer(1)->raceFinished);
            ASSERT_TRUE(client.getPlayer(0)->raceFinished);
            ASSERT_EQ(client.getPlayer(1)->placement, 1);
            ASSERT_EQ(client.getPlayer(0)->placement, 2);
        }

        client.disconnectNetwork();
        host.disconnectNetwork();
    }

    // -------------------------------------------------------------
    // Test 9: Rematch & Return to Lobby
    // -------------------------------------------------------------
    TEST_CASE("MP1.9: Host triggers rematch returning all players to lobby") {
        NetworkManager host;
        host.startHosting("REMATCH", "HOST", 0);
        processQtEvents(30);

        NetworkManager client;
        client.startJoining("REMATCH", "CLIENT", "127.0.0.1", host.getServerPort());

        for (int i = 0; i < 20; ++i) {
            processQtEvents(40);
            if (client.isConnected() && client.getPlayers().size() == 2) break;
        }

        bool clientReturned = false;
        QObject::connect(&client, &NetworkManager::returnedToLobby, [&]() {
            clientReturned = true;
        });

        host.returnToLobby();
        processQtEvents(50);

        ASSERT_TRUE(clientReturned);
        // Code should be preserved for seamless rematch
        ASSERT_EQ(host.getSecretCode(), QString("REMATCH"));
        ASSERT_EQ(client.getSecretCode(), QString("REMATCH"));

        client.disconnectNetwork();
        host.disconnectNetwork();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    runMultiplayerTests();
    TestContext::instance().printSummary();
    return (TestContext::instance().failedAssertions() == 0) ? 0 : 1;
}
