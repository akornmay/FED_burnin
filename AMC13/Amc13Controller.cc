#include "Amc13Controller.h"

Amc13Controller::Amc13Controller()
{
    fAmc13Interface = nullptr;
    fAmc13 = nullptr;
}

Amc13Controller::~Amc13Controller()
{
    delete fAmc13;
    delete fAmc13Interface;
}

void Amc13Controller::InitializeAmc13( const std::string& pFilename, std::ostream& os )
{
    if ( pFilename.find( ".xml" ) != std::string::npos )
        parseAmc13xml(pFilename, os);
    else std::cerr << "Could not parse settings file " << pFilename << " - it is not .xml!" << std::endl;
}

void Amc13Controller::ConfigureAmc13( std::ostream& os )
{
    os << BOLDGREEN << "Configuring Amc13!" << RESET << std::endl;
    // no need to pass the Amc13 memory description as there is only 1, I just keep it for reference and update accordingly!
    //fAmc13Interface->ConfigureAmc13();
}

void Amc13Controller::parseAmc13xml(const std::string& pFilename, std::ostream& os)
{
    pugi::xml_document doc;
    int i, j;

    pugi::xml_parse_result result = doc.load_file( pFilename.c_str() );

    if ( !result )
    {
        os << "ERROR :\n Unable to open the file : " << pFilename << std::endl;
        os << "Error description : " << result.description() << std::endl;
        return;
    }

    os << "\n";
    for ( i = 0; i < 80; i++ )
        os << "*";
    os << "\n";
    for ( j = 0; j < 40; j++ )
        os << " ";
    os << BOLDRED << "AMC13 Settings " << RESET << std::endl;
    for ( i = 0; i < 80; i++ )
        os << "*";
    os << "\n";
    os << "\n";

    // no clue why I have to loop but that's what it is!
    for ( pugi::xml_node cAmc13node = doc.child( "HwDescription" ).child( "AMC13" ); cAmc13node; cAmc13node = cAmc13node.next_sibling("AMC13") )
    {
        os << BOLDCYAN << cAmc13node.name() << RESET << std::endl;
        // now create a new AMC13 Description Object
        if ( fAmc13 != nullptr ) delete fAmc13;
        fAmc13 = new Amc13Description();

        std::string cUri1, cUri2;
        std::string cAddressT1, cAddressT2;
        // finally find the connection nodes and construct the proper Amc13FWInterface
        for ( pugi::xml_node Amc13Connection = cAmc13node.child( "connection" ); Amc13Connection; Amc13Connection = Amc13Connection.next_sibling("connection") )
        {
            if ( std::string(Amc13Connection.attribute( "id" ).value()) == "T1")
            {
                cUri1 = std::string(Amc13Connection.attribute( "uri" ).value());
                cAddressT1 = std::string(Amc13Connection.attribute( "address_table" ).value());
            }
            else if ( std::string(Amc13Connection.attribute( "id" ).value()) == "T2" )
            {

                cUri2 = std::string(Amc13Connection.attribute( "uri" ).value());
                cAddressT2 = std::string(Amc13Connection.attribute( "address_table" ).value());
            }
            os << BOLDBLUE <<  "|" << "----" << "Board Id: " << BOLDYELLOW << Amc13Connection.attribute("id").value() << BOLDBLUE << " URI: " << BOLDYELLOW << Amc13Connection.attribute("uri").value() << BOLDBLUE << " Address Table: " << BOLDYELLOW << Amc13Connection.attribute("address_table").value() << RESET << std::endl;
        }

        // here loop over the Register nodes and add them to the AMC13Description object!
        for ( pugi::xml_node Amc13RegNode = cAmc13node.child( "Register" ); Amc13RegNode; Amc13RegNode = Amc13RegNode.next_sibling("Register") )
        {
            std::string regname = std::string(Amc13RegNode.attribute( "name" ).value());

            os << BOLDCYAN << "|" << "----" << Amc13RegNode.name() << "  " << Amc13RegNode.attribute( "tounge" ).name() << " : " << Amc13RegNode.attribute( "tounge" ).value() << " - name: " << regname << " " << BOLDRED << atoi(Amc13RegNode.first_child().value()) << RESET << std:: endl;
            if ( std::string(Amc13RegNode.attribute("tounge").value()) == "T1" )
            {
                fAmc13->setReg(1 ,  regname, convertAnyInt( Amc13RegNode.first_child().value() ) );
            }
            else if ( std::string(Amc13RegNode.attribute("tounge").value()) == "T2" )
            {
                fAmc13->setReg(2 ,  regname, convertAnyInt( Amc13RegNode.first_child().value() ) );
            }
        }

        // now get all the properties and add to the Amc13Description object
        fAmc13->setAMCMask( parseAMCMask( cAmc13node.child( "AMCmask" ), std::cout ) );
        fAmc13->setTrigger( parseTrigger( cAmc13node.child( "Trigger" ), std::cout ) );

        for (pugi::xml_node cBGOnode = cAmc13node.child( "BGO" ); cBGOnode; cBGOnode = cBGOnode.next_sibling("BGO"))
        {
            fAmc13->addBGO( parseBGO( cBGOnode, std::cout ) );
        }

        // now instantiate the AMC13Interface
        if ( fAmc13Interface != nullptr ) delete fAmc13Interface;
        fAmc13Interface = new Amc13Interface(cUri1, cAddressT1, cUri2, cAddressT2);
    }
    os << "\n";
}

std::vector<int> Amc13Controller::parseAMCMask(pugi::xml_node pNode,  std::ostream & os)
{
    std::vector<int> cVec;
    if (std::string(pNode.name()) == "AMCmask")
    {
        //for ( pugi::xml_node cNode = pNode.child( "AMCmask" ); cNode; cNode = cNode.next_sibling("AMCmask") )
        //{
        std::string cList = std::string(pNode.attribute("enable").value());
        std::string ctoken;
        std::stringstream cStr(cList);

        os <<  BOLDCYAN << "|" << "----" << "List of Enabled AMCs: ";
        while (std::getline(cStr, ctoken, ','))
        {
            cVec.push_back(convertAnyInt( ctoken.c_str() ));
            os << GREEN << ctoken << BOLDCYAN << ", ";
        }
        os << RESET << std::endl;
    }
    return cVec;
}

BGO* Amc13Controller::parseBGO(pugi::xml_node pNode,  std::ostream & os)
{
    BGO* cBGO = nullptr;
    if ( std::string(pNode.name()) == "BGO")
    {
        cBGO = new BGO( atoi(pNode.attribute( "channel" ).value()), bool(pNode.attribute( "enable" ).value()), atoi(pNode.attribute( "prescale" ).value() ), atoi(pNode.attribute( "bx" ).value()) );
        os <<  BOLDCYAN << "|" << "----" << "BGO : channel " << cBGO->fChannel << " status[" << cBGO->fStatus << "] Prescale " << cBGO->fPrescale << " start BX " << cBGO->fBX << RESET << std::endl;
    }
    return cBGO;
}

Trigger* Amc13Controller::parseTrigger( pugi::xml_node pNode, std::ostream & os )
{
    Trigger* cTrg = nullptr;
    if ( std::string(pNode.name()) == "Trigger" )
    {
        cTrg = new Trigger( atoi(pNode.attribute( "type" ).value()), atoi( pNode.attribute( "rate" ).value()), atoi( pNode.attribute( "rules" ).value() ) );
        os <<  BOLDGREEN << "|" << "----" << "Trigger Config : Type " << cTrg->fType << " Rate " << cTrg->fRate << " Rules " << cTrg->fRules << RESET << std::endl;

    }
    return cTrg;
}
