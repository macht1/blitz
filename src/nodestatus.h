#ifndef NODESTATUS_H
#define NODESTATUS_H

class NodeStatus {
	public:
		bool IsStandard();
		bool IsGold();
		bool IsElite();
		bool IsBlitz();
		void NodeStatus();
		
	private:
		void GetTXID();
		void GetTime();
}
#endif // NODESTATUS_H
