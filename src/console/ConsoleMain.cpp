#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include <iostream>

#include "src/data/database/Database.hpp"
#include "src/infrastructure/config/AppConfig.hpp"

namespace {
// exactly the banner that was asked for
const char* banner = R"(
  .    .::::::.:::.    :::.:::      .::.
 ;;; .;;,.;;;`    ``;;;;,  `;;;';;,   ,;;;'
 [[[[[/'  '[==/[[[[, [[[[[. '[[ \[[  .[[/
_$$$$,      '''    $ $$$ "Y$c$$  Y$c.$$"
"888"88o,  88b    dP 888    Y88   Y88P
 MMM "MMP"  "YMmMY"  MMM     YM    MP
)";

// only these can be requested, no raw sql from the prompt
const QStringList allowedTables = {
    "users", "sessions", "settings", "teams", "projects",
    "project_members", "project_links", "project_files",
    "tasks", "countdowns", "permissions", "audit_logs"
};

const int maxCellWidth = 40;
const int maxRows = 200;

void printHelp()
{
    std::cout <<
        "commands:\n"
        "  help              this list\n"
        "  status            is the database still up\n"
        "  request <table>   show rows, e.g.  request users\n"
        "  exit              quit\n"
        "\n"
        "tables: users, sessions, settings, teams, projects,\n"
        "        project_members, project_links, project_files,\n"
        "        tasks, countdowns, permissions, audit_logs\n";
}

QString cellText(const QVariant& value)
{
    if (value.isNull())
        return "-";
    if (value.typeId() == QMetaType::Bool)
        return value.toBool() ? "yes" : "no";
    return value.toString();
}

void requestTable(const QString& table)
{
    const QString name = table.trimmed().toLower();
    if (name.isEmpty()) {
        std::cout << "usage: request <table>  (try: request users)\n";
        return;
    }
    if (!allowedTables.contains(name)) {
        std::cout << "don't know the table '" << name.toStdString() << "'. try 'help' for the list.\n";
        return;
    }

    QSqlQuery query;
    if (!query.exec(QString("select * from %1 order by 1 limit %2")
                        .arg(name)
                        .arg(maxRows))) {
        std::cout << "query failed: "
                  << query.lastError().text().toStdString() << "\n";
        return;
    }

    const QSqlRecord record = query.record();
    if (record.count() == 0) {
        std::cout << "the table has no columns? weird.\n";
        return;
    }

    // header + column widths
    QStringList headers;
    QVector<int> widths;
    for (int i = 0; i < record.count(); ++i) {
        headers << record.fieldName(i);
        widths << qMin(record.fieldName(i).size(), maxCellWidth);
    }

    QVector<QStringList> rows;
    while (query.next()) {
        QStringList cells;
        for (int i = 0; i < record.count(); ++i) {
            QString text = cellText(query.value(i));
            if (text.size() > maxCellWidth)
                text = text.left(maxCellWidth - 1) + "\u2026";
            cells << text;
            widths[i] = qMax(widths[i], text.size());
        }
        rows << cells;
    }

    if (rows.isEmpty()) {
        std::cout << "(no rows in " << name.toStdString() << ")\n";
        return;
    }

    QString line;
    for (int i = 0; i < headers.size(); ++i)
        line += headers[i].leftJustified(widths[i]) + "  ";
    std::cout << line.toStdString() << "\n";

    line.clear();
    for (int i = 0; i < headers.size(); ++i)
        line += QString(widths[i], '-') + "  ";
    std::cout << line.toStdString() << "\n";

    for (const QStringList& cells : rows) {
        line.clear();
        for (int i = 0; i < cells.size(); ++i)
            line += cells[i].leftJustified(widths[i]) + "  ";
        std::cout << line.toStdString() << "\n";
    }

    std::cout << "(" << rows.size() << " row" << (rows.size() == 1 ? "" : "s")
              << ", showing up to " << maxRows << ")\n";
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("OrbitConsole");
    QCoreApplication::setOrganizationName("Kasnovitch");

    std::cout << banner << "\n";
    std::cout << "orbit batch console - postgres. type 'help' for commands.\n\n";

    // one connection for the whole run, same config file as the gui
    AppConfig config;
    Database database;
    if (!config.load()) {
        std::cout << "database: " << config.error().toStdString() << "\n";
    } else if (!database.open(config.database())) {
        std::cout << database.status().toStdString() << "\n";
    } else {
        database.ensureTables();
        std::cout << database.status().toStdString() << "\n";
    }

    std::cout << "\norbit> " << std::flush;

    std::string line;
    while (std::getline(std::cin, line)) {
        // trim
        const auto first = line.find_first_not_of(" \t\r\n");
        const auto last = line.find_last_not_of(" \t\r\n");
        const std::string cmd = (first == std::string::npos)
            ? std::string()
            : line.substr(first, last - first + 1);

        if (!cmd.empty()) {
            if (cmd == "exit" || cmd == "quit")
                break;

            const auto space = cmd.find(' ');
            const std::string verb = (space == std::string::npos) ? cmd : cmd.substr(0, space);
            const std::string rest = (space == std::string::npos) ? "" : cmd.substr(space + 1);

            if (verb == "help") {
                printHelp();
            } else if (verb == "status") {
                std::cout << database.status().toStdString() << "\n";
            } else if (verb == "request") {
                requestTable(QString::fromStdString(rest));
            } else {
                std::cout << "don't know '" << verb << "' - type 'help'.\n";
            }
        }

        std::cout << "\norbit> " << std::flush;
    }

    std::cout << "bye.\n";
    return 0;
}
