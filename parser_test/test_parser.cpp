#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "IndigoSequenceParser.h"
#include "SequenceItemModel.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("SequenceParserTest");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Test for IndigoSequenceParser");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption inputOption(QStringList() << "i" << "input", "Input script file", "input_file");
    parser.addOption(inputOption);
    
    QCommandLineOption outputOption(QStringList() << "o" << "output", "Output script file", "output_file");
    parser.addOption(outputOption);
    
    parser.process(app);
    
    if (!parser.isSet(inputOption)) {
        qCritical() << "Error: Input file not specified";
        parser.showHelp(1);
        return 1;
    }
    
    QString inputFilePath = parser.value(inputOption);
    QString outputFilePath = parser.isSet(outputOption) ? parser.value(outputOption) : "output.js";
    
    // Read input file
    QFile inputFile(inputFilePath);
    if (!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Error: Could not open input file:" << inputFilePath;
        return 1;
    }
    
    QTextStream in(&inputFile);
    QString script = in.readAll();
    inputFile.close();
    
    qInfo() << "Parsing script from:" << inputFilePath;
    
    // Parse script
    IndigoSequenceParser sequenceParser;
    
    // Connect to validation error signal to display errors
    QObject::connect(&sequenceParser, &IndigoSequenceParser::validationError, 
                     [](const QString& error) { 
                         qWarning() << "Validation error:" << error; 
                     });
    
    QVector<FunctionCall> calls = sequenceParser.parse(script);
    qInfo() << "Parsed" << calls.size() << "function calls";
    
    // Validate calls
    bool isValid = sequenceParser.validateCalls(calls);
    if (!isValid) {
        qWarning() << "Script validation failed";
    } else {
        qInfo() << "Script validation successful";
    }
    
    // Generate script
    QString generatedScript = sequenceParser.generate(calls);
    
    // Write to output file
    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Error: Could not open output file:" << outputFilePath;
        return 1;
    }
    
    QTextStream out(&outputFile);
    out << generatedScript;
    outputFile.close();
    
    qInfo() << "Generated script written to:" << outputFilePath;
    
    return 0;
}